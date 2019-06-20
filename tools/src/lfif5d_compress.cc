/******************************************************************************\
* SOUBOR: lfif5d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"
#include "file_mask.h"

#include <colorspace.h>
#include <lfif_encoder.h>
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

bool is_square(uint64_t num) {
  for (size_t i = 0; i * i <= num; i++) {
    if (i * i == num) {
      return true;
    }
  }

  return false;
}

int main(int argc, char *argv[]) {
  const char *input_file_mask  {};
  const char *output_file_name {};
  float quality                {};

  vector<uint8_t> rgb_data     {};

  uint64_t width               {};
  uint64_t height              {};
  uint64_t frames_count        {};
  uint64_t views_count         {};
  uint32_t max_rgb_value       {};

  LfifEncoder<BS, 5> *encoder  {};
  ofstream            output   {};

  PPMFileStruct ppm {};

  if (!parse_args(argc, argv, input_file_mask, output_file_name, quality)) {
    return 1;
  }

  for (size_t frame = 0; frame < get_mask_names_count(input_file_mask, '@'); frame++) {
    size_t v_count {};

    for (size_t view = 0; view < get_mask_names_count(input_file_mask, '#'); view++) {
      ppm.file = fopen(get_name_from_mask(get_name_from_mask(input_file_mask, '@', frame), '#', view).c_str(), "rb");
      if (!ppm.file) {
        continue;
      }

      v_count++;

      if (readPPMHeader(&ppm)) {
        cerr << "ERROR: BAD PPM HEADER" << endl;
        return 1;
      }

      fclose(ppm.file);

      if (width && height && max_rgb_value) {
        if ((ppm.width != width) || (ppm.height != height) || (ppm.color_depth != max_rgb_value)) {
          cerr << "ERROR: PPMs DIMENSIONS MISMATCH" << endl;
          return 1;
        }
      }

      width       = ppm.width;
      height      = ppm.height;
      max_rgb_value = ppm.color_depth;
    }

    if (v_count) {
      if (!views_count) {
        views_count = v_count;
      }
      else if (v_count != views_count) {
        cerr << "ERROR: VIEW COUNT MISMATCH" << endl;
        return 1;
      }

      frames_count++;
    }

  }

  if (!views_count || !frames_count) {
    cerr << "ERROR: NO IMAGE LOADED" << endl;
    return 1;
  }

  if (!is_square(views_count)) {
    cerr << "ERROR: NOT SQUARE" << endl;
    return 1;
  }

  size_t input_size = width * height * views_count * 3 * BLOCK_SIZE;
  input_size *= (max_rgb_value > 255) ? 2 : 1;
  rgb_data.resize(input_size);

  size_t last_slash_pos = string(output_file_name).find_last_of('/');
  if (last_slash_pos != string::npos) {
    string command = "mkdir -p " + string(output_file_name).substr(0, last_slash_pos);
    system(command.c_str());
  }

  output.open(output_file_name);
  if (!output) {
    cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  encoder = new LfifEncoder<BS, 5>;

  encoder->img_dims[0] = width;
  encoder->img_dims[1] = height;
  encoder->img_dims[2] = sqrt(views_count);
  encoder->img_dims[3] = sqrt(views_count);
  encoder->img_dims[4] = frames_count;
  encoder->img_dims[5] = 1;
  encoder->color_depth = ceil(log2(max_rgb_value + 1));

  int64_t loaded_fifth_dimension_block_index = -1;
  auto inputF0 = [&](size_t channel, size_t index) -> RGBUNIT {
    size_t indexed_fifth_dimension_block_index = index / (width * height * views_count * BLOCK_SIZE);

    if (static_cast<int64_t>(indexed_fifth_dimension_block_index) != loaded_fifth_dimension_block_index) {
      cerr << "INFO: LOADING NEW FRAMES" << endl;

      Pixel *ppm_row              {};
      size_t skipped_frames_count {};
      size_t loaded_frames_count  {};

      ppm_row = allocPPMRow(width);

      for (size_t frame = 0; frame < get_mask_names_count(input_file_mask, '@') && loaded_frames_count < BLOCK_SIZE; frame++) {

        size_t loaded_views_count {};
        for (size_t view = 0; view < get_mask_names_count(input_file_mask, '#'); view++) {
          ppm.file = fopen(get_name_from_mask(get_name_from_mask(input_file_mask, '@', frame), '#', view).c_str(), "rb");
          if (!ppm.file) {
            continue;
          }
          else if (skipped_frames_count < indexed_fifth_dimension_block_index * BLOCK_SIZE) {
            fclose(ppm.file);
            skipped_frames_count++;
            break;
          }

          readPPMHeader(&ppm);

          for (size_t row = 0; row < ppm.height; row++) {
            if (readPPMRow(&ppm, ppm_row)) {
              cerr << "ERROR: BAD PPM" << endl;
              fclose(ppm.file);
            }

            if (ppm.color_depth < 256) {
              for (size_t col = 0; col < ppm.width; col++) {
                reinterpret_cast<uint8_t *>(rgb_data.data())[(((loaded_frames_count * views_count + loaded_views_count) * ppm.height + row) * ppm.width + col) * 3 + 0] = ppm_row[col].r;
                reinterpret_cast<uint8_t *>(rgb_data.data())[(((loaded_frames_count * views_count + loaded_views_count) * ppm.height + row) * ppm.width + col) * 3 + 1] = ppm_row[col].g;
                reinterpret_cast<uint8_t *>(rgb_data.data())[(((loaded_frames_count * views_count + loaded_views_count) * ppm.height + row) * ppm.width + col) * 3 + 2] = ppm_row[col].b;
              }
            }
            else {
              for (size_t col = 0; col < ppm.width; col++) {
                reinterpret_cast<uint16_t *>(rgb_data.data())[(((loaded_frames_count * views_count + loaded_views_count) * ppm.height + row) * ppm.width + col) * 3 + 0] = ppm_row[col].r;
                reinterpret_cast<uint16_t *>(rgb_data.data())[(((loaded_frames_count * views_count + loaded_views_count) * ppm.height + row) * ppm.width + col) * 3 + 1] = ppm_row[col].g;
                reinterpret_cast<uint16_t *>(rgb_data.data())[(((loaded_frames_count * views_count + loaded_views_count) * ppm.height + row) * ppm.width + col) * 3 + 2] = ppm_row[col].b;
              }
            }

          }

          fclose(ppm.file);
          loaded_views_count++;
        }

        if (loaded_views_count == views_count) {
          loaded_frames_count++;
        }
        else if (loaded_views_count) {
          cerr << "ERROR: THIS SHOULD NEVER HAPPEN" << endl;
        }
      }

      freePPMRow(ppm_row);

      loaded_fifth_dimension_block_index = indexed_fifth_dimension_block_index;
    }

    if (max_rgb_value < 256) {
      return reinterpret_cast<const uint8_t *>(rgb_data.data())[index * 3 + channel];
    }
    else {
      return reinterpret_cast<const uint16_t *>(rgb_data.data())[index * 3 + channel];
    }
  };

  auto inputF = [&](size_t index) -> INPUTTRIPLET {
    RGBUNIT R = inputF0(0, index);
    RGBUNIT G = inputF0(1, index);
    RGBUNIT B = inputF0(2, index);

    INPUTUNIT  Y = YCbCr::RGBToY(R, G, B) - (max_rgb_value + 1) / 2;
    INPUTUNIT Cb = YCbCr::RGBToCb(R, G, B);
    INPUTUNIT Cr = YCbCr::RGBToCr(R, G, B);

    return {Y, Cb, Cr};
  };

  initEncoder(*encoder);
  constructQuantizationTables(*encoder, "DEFAULT", quality);
  referenceScan(*encoder, inputF); //FIRST IMAGE SCAN
  constructTraversalTables(*encoder, "DEFAULT");
  huffmanScan(*encoder, inputF); //SECOND IMAGE SCAN
  constructHuffmanTables(*encoder);
  writeHeader(*encoder, output);
  outputScan(*encoder, inputF, output); //THIRD IMAGE SCAN

  delete encoder;

  return 0;
}
