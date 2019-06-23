/******************************************************************************\
* SOUBOR: lfif5d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"
#include "file_mask.h"

#include <colorspace.h>
#include <lfif_encoder.h>
#include <ppm.h>

#include <getopt.h>

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

bool parse_args_video(int argc, char *argv[], const char *&input_file_mask, const char *&output_file_name, float &quality, size_t &start_frame) {
  const char *arg_quality     {};
  const char *arg_start_frame {};

  char opt;
  while ((opt = getopt(argc, argv, "i:o:q:s:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
          continue;
        }
        break;

      case 'o':
        if (!output_file_name) {
          output_file_name = optarg;
          continue;
        }
        break;

      case 'q':
        if (!arg_quality) {
          arg_quality = optarg;
          continue;
        }
        break;

      case 's':
        if (!arg_start_frame) {
          arg_start_frame = optarg;
          continue;
        }
        break;

      default:
        break;
    }

    print_usage(argv[0]);
    return false;
  }

  if ((!input_file_mask) || (!output_file_name) || (!arg_quality)) {
    print_usage(argv[0]);
    return false;
  }

  float tmp_quality = atof(arg_quality);
  if ((tmp_quality < 1.f) || (tmp_quality > 100.f)) {
    print_usage(argv[0]);
    return false;
  }
  quality = tmp_quality;

  if (arg_start_frame) {
    int tmp_start_frame = atoi(arg_start_frame);
    if (tmp_start_frame < 0) {
      print_usage(argv[0]);
      return false;
    }
    start_frame = tmp_start_frame;
  }
  else {
    start_frame = 0;
  }

  return true;
}


int main(int argc, char *argv[]) {
  const char *input_file_mask  {};
  const char *output_file_name {};
  float quality                {};
  size_t start_frame           {};

  vector<uint8_t> rgb_data     {};

  uint64_t width               {};
  uint64_t height              {};
  uint64_t frames_count        {};
  uint64_t views_count         {};
  uint32_t max_rgb_value       {};

  LfifEncoder<BS, 5> *encoder  {};
  ofstream            output   {};

  PPMFileStruct ppm {};

  if (!parse_args_video(argc, argv, input_file_mask, output_file_name, quality, start_frame)) {
    return 1;
  }

  cerr << "INFO: CHECKING FRAMES" << endl;
  for (size_t frame = start_frame; frame < get_mask_names_count(input_file_mask, '@'); frame++) {
    cerr << "INFO: CHECKING FRAME " << frame << endl;
    size_t v_count {};

    for (size_t view = 0; view < get_mask_names_count(input_file_mask, '#'); view++) {
      ppm.file = fopen(get_name_from_mask(get_name_from_mask(input_file_mask, '@', frame), '#', view).c_str(), "rb");
      if (!ppm.file) {
        continue;
      }

      v_count++;

      if (readPPMHeader(&ppm)) {
        fclose(ppm.file);
        cerr << "ERROR: BAD PPM HEADER" << endl;
        return 1;
      }

      fclose(ppm.file);

      if (width && height && max_rgb_value) {
        if ((ppm.width != width) || (ppm.height != height) || (ppm.color_depth != max_rgb_value)) {
          cerr << "ERROR: PPM DIMENSIONS MISMATCH" << endl;
          return 1;
        }
      }

      width         = ppm.width;
      height        = ppm.height;
      max_rgb_value = ppm.color_depth;
    }

    if (!v_count) {
      break;
    }

    if (!views_count) {
      views_count = v_count;
    }
    else if (v_count != views_count) {
      cerr << "ERROR: VIEW COUNT MISMATCH" << endl;
      return 1;
    }

    frames_count++;
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

  auto load_frames = [&](size_t first_frame_index) {
    cerr << "INFO: LOADING FRAMES " << first_frame_index << " - " << first_frame_index + BLOCK_SIZE - 1 << endl;

    Pixel *ppm_row              {};
    size_t loaded_frames_count  {};

    ppm_row = allocPPMRow(width);

    for (size_t frame = first_frame_index; (frame < (start_frame + frames_count)) && (frame < (first_frame_index + BLOCK_SIZE)); frame++) {
      cerr << "INFO: LOADING FRAME " << frame << ": " << get_name_from_mask(input_file_mask, '@', frame) << endl;
      size_t loaded_views_count {};

      for (size_t view = 0; view < get_mask_names_count(input_file_mask, '#'); view++) {
        ppm.file = fopen(get_name_from_mask(get_name_from_mask(input_file_mask, '@', frame), '#', view).c_str(), "rb");
        if (!ppm.file) {
          continue;
        }

        readPPMHeader(&ppm);

        for (size_t row = 0; row < ppm.height; row++) {
          if (readPPMRow(&ppm, ppm_row)) {
            cerr << "ERROR: BAD PPM" << endl;
            fclose(ppm.file);
          }

          if (ppm.color_depth < 256) {
            for (size_t col = 0; col < ppm.width; col++) {
              uint8_t *data_ptr = reinterpret_cast<uint8_t *>(rgb_data.data());

              data_ptr[(((loaded_frames_count * views_count + loaded_views_count) * ppm.height + row) * ppm.width + col) * 3 + 0] = ppm_row[col].r;
              data_ptr[(((loaded_frames_count * views_count + loaded_views_count) * ppm.height + row) * ppm.width + col) * 3 + 1] = ppm_row[col].g;
              data_ptr[(((loaded_frames_count * views_count + loaded_views_count) * ppm.height + row) * ppm.width + col) * 3 + 2] = ppm_row[col].b;
            }
          }
          else {
            for (size_t col = 0; col < ppm.width; col++) {
              uint16_t *data_ptr = reinterpret_cast<uint16_t *>(rgb_data.data());

              data_ptr[(((loaded_frames_count * views_count + loaded_views_count) * ppm.height + row) * ppm.width + col) * 3 + 0] = ppm_row[col].r;
              data_ptr[(((loaded_frames_count * views_count + loaded_views_count) * ppm.height + row) * ppm.width + col) * 3 + 1] = ppm_row[col].g;
              data_ptr[(((loaded_frames_count * views_count + loaded_views_count) * ppm.height + row) * ppm.width + col) * 3 + 2] = ppm_row[col].b;
            }
          }

        }

        fclose(ppm.file);
        loaded_views_count++;
      }

      if (loaded_views_count == views_count) {
        loaded_frames_count++;
        cerr << "INFO: FRAME " << loaded_frames_count << "/" << BLOCK_SIZE << " LOADED" << endl;

        cerr << (int)rgb_data[122944 * 3 + 0] << ", " << (int)[122944 * 3 + 1] << ", " << (int)[122944 * 3 + 2] << endl;
        cerr << (int)rgb_data[122944 * 3 + 3] << ", " << (int)[122944 * 3 + 4] << ", " << (int)[122944 * 3 + 5] << endl;

      }
      else if (loaded_views_count) {
        cerr << "ERROR: THIS SHOULD NEVER HAPPEN" << endl;
      }
    }

    freePPMRow(ppm_row);

    cerr << "INFO: FRAMES " << first_frame_index << " - " << first_frame_index + BLOCK_SIZE - 1 << " WERE LOADED" << endl;
  };

  int64_t loaded_fifth_dimension_block_index = -1;

  auto inputF0 = [&](size_t channel, size_t index) -> RGBUNIT {
    size_t indexed_fifth_dimension_block_index = index / (width * height * views_count * BLOCK_SIZE);

    if (static_cast<int64_t>(indexed_fifth_dimension_block_index) != loaded_fifth_dimension_block_index) {
      load_frames(start_frame + (indexed_fifth_dimension_block_index * BLOCK_SIZE));
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
  constructTraversalTables(*encoder, "ZIGZAG");
  huffmanScan(*encoder, inputF);
  constructHuffmanTables(*encoder);
  writeHeader(*encoder, output);
  outputScan(*encoder, inputF, output);

  delete encoder;

  return 0;
}
