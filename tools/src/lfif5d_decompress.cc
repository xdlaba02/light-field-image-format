/******************************************************************************\
* SOUBOR: lfif5d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "decompress.h"
#include "file_mask.h"

#include <lfif_decoder.h>
#include <colorspace.h>
#include <ppm.h>

#include <cmath>

#include <iostream>
#include <vector>

#ifdef BLOCK_SIZE
const size_t BS = BLOCK_SIZE;
#else
const size_t BS = 8;
#endif

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_file_name  {};
  const char *output_file_mask {};

  vector<uint8_t> rgb_data     {};

  LfifDecoder<BS, 5> *decoder  {};
  ifstream            input    {};

  uint16_t max_rgb_value       {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return 1;
  }

  input.open(input_file_name);
  if (!input) {
    cerr << "ERROR: CANNON OPEN " << input_file_name << " FOR READING\n";
    return 1;
  }

  decoder = new LfifDecoder<BS, 5>;

  if (readHeader(*decoder, input)) {
    cerr << "ERROR: IMAGE HEADER INVALID\n";
    return 2;
  }

  max_rgb_value = pow(2, decoder->color_depth) - 1;

  initDecoder(*decoder);

  size_t output_buffer_size = decoder->img_dims[0] * decoder->img_dims[1] * decoder->img_dims[2] * decoder->img_dims[3] * BLOCK_SIZE * 3;
  output_buffer_size *= (decoder->color_depth > 8) ? 2 : 1;
  rgb_data.resize(output_buffer_size);

  size_t views_count = decoder->img_dims[2] * decoder->img_dims[3];

  auto flush_frames = [&](size_t first_frame_index) {
    cerr << "INFO: FLUSHING FRAMES " << first_frame_index << " - " << first_frame_index + BLOCK_SIZE - 1 << endl;

    PPMFileStruct ppm            {};
    Pixel *ppm_row               {};
    size_t flushed_frames_count  {};

    ppm.width = decoder->img_dims[0];
    ppm.height = decoder->img_dims[1];
    ppm.color_depth = max_rgb_value;

    ppm_row = allocPPMRow(ppm.width);

    size_t last_slash_pos = string(output_file_mask).find_last_of('/');

    for (size_t frame = first_frame_index; (frame < decoder->img_dims[4]) && (frame < (first_frame_index + BLOCK_SIZE)); frame++) {
      cerr << "INFO: FLUSHING FRAME " << frame << ": " << get_name_from_mask(output_file_mask, '@', frame) << endl;

      for (size_t view = 0; view < views_count; view++) {

        std::string filename = get_name_from_mask(get_name_from_mask(output_file_mask, '@', frame), '#', view);

        if (last_slash_pos != string::npos) {
          string command("mkdir -p " + filename.substr(0, last_slash_pos));
          system(command.c_str());
        }

        ppm.file = fopen(filename.c_str(), "wb");
        if (!ppm.file) {
          cerr << "ERROR: CANNOT OPEN " << filename << " FOR WRITING" << endl;
          exit(1);
        }

        if (writePPMHeader(&ppm)) {
          cerr << "ERROR: CANNOT WRITE TO " << filename << endl;
          exit(1);
        }

        for (size_t row = 0; row < ppm.height; row++) {
          if (decoder->color_depth > 8) {
            for (size_t col = 0; col < ppm.width; col++) {
              const uint16_t *data_ptr = reinterpret_cast<const uint16_t *>(rgb_data.data());
              ppm_row[col].r = data_ptr[(((flushed_frames_count * views_count + view) * ppm.height + row) * ppm.width + col) * 3 + 0];
              ppm_row[col].g = data_ptr[(((flushed_frames_count * views_count + view) * ppm.height + row) * ppm.width + col) * 3 + 1];
              ppm_row[col].b = data_ptr[(((flushed_frames_count * views_count + view) * ppm.height + row) * ppm.width + col) * 3 + 2];
            }
          }
          else {
            for (size_t col = 0; col < ppm.width; col++) {
              const uint8_t *data_ptr = reinterpret_cast<const uint8_t *>(rgb_data.data());
              ppm_row[col].r = data_ptr[(((flushed_frames_count * views_count + view) * ppm.height + row) * ppm.width + col) * 3 + 0];
              ppm_row[col].g = data_ptr[(((flushed_frames_count * views_count + view) * ppm.height + row) * ppm.width + col) * 3 + 1];
              ppm_row[col].b = data_ptr[(((flushed_frames_count * views_count + view) * ppm.height + row) * ppm.width + col) * 3 + 2];
            }
          }


          if (writePPMRow(&ppm, ppm_row)) {
            cerr << "ERROR: CANNOT WRITE TO " << filename << endl;
            exit(1);
          }
        }

        fclose(ppm.file);
      }

      flushed_frames_count++;
    }

    freePPMRow(ppm_row);
  };

  size_t decoded_fifth_dimension_block_index = 0;

  auto outputF0 = [&](size_t channel, size_t index, RGBUNIT value) {

    size_t indexed_fifth_dimension_block_index = index / (decoder->img_dims[0] * decoder->img_dims[1] * decoder->img_dims[2] * decoder->img_dims[3] * BLOCK_SIZE);

    if (indexed_fifth_dimension_block_index != decoded_fifth_dimension_block_index) {
      flush_frames(decoded_fifth_dimension_block_index * BLOCK_SIZE);
      decoded_fifth_dimension_block_index = indexed_fifth_dimension_block_index;
    }

    if (decoder->color_depth > 8) {
      reinterpret_cast<uint16_t *>(rgb_data.data())[index * 3 + channel] = value;
    }
    else {
      reinterpret_cast<uint8_t *>(rgb_data.data())[index * 3 + channel] = value;
    }
  };

  auto outputF = [&](size_t index, const INPUTTRIPLET &triplet) {
    INPUTUNIT  Y = triplet[0] + ((max_rgb_value + 1) / 2);
    INPUTUNIT Cb = triplet[1];
    INPUTUNIT Cr = triplet[2];

    RGBUNIT R = clamp<INPUTUNIT>(round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, max_rgb_value);
    RGBUNIT G = clamp<INPUTUNIT>(round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, max_rgb_value);
    RGBUNIT B = clamp<INPUTUNIT>(round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, max_rgb_value);

    outputF0(0, index, R);
    outputF0(1, index, G);
    outputF0(2, index, B);
  };

  decodeScan(*decoder, input, outputF);

  flush_frames(decoded_fifth_dimension_block_index * BLOCK_SIZE);

  delete decoder;

  return 0;
}
