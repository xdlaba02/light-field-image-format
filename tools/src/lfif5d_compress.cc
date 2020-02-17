/******************************************************************************\
* SOUBOR: lfif5d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"
#include "file_mask.h"
#include "plenoppm.h"

#include <lfif_encoder.h>
#include <ppm.h>

#include <getopt.h>

#include <cmath>

#include <iostream>
#include <vector>

using namespace std;

bool is_square(uint64_t num) {
  for (size_t i = 0; i * i <= num; i++) {
    if (i * i == num) {
      return true;
    }
  }

  return false;
}

bool parse_args_video(int argc, char *argv[], const char *&input_file_mask, const char *&output_file_name, float &quality, size_t &start_frame, bool &predict) {
  const char *arg_quality     {};
  const char *arg_start_frame {};

  input_file_mask  = nullptr;
  output_file_name = nullptr;
  quality          = 0;
  start_frame      = 0;
  predict          = false;

  char opt;
  while ((opt = getopt(argc, argv, "i:o:q:s:p")) >= 0) {
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

      case 'p':
        if (!predict) {
          predict = true;
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

  bool predict                 {};

  vector<PPM> ppm_data         {};

  uint64_t width               {};
  uint64_t height              {};
  uint32_t max_rgb_value       {};

  LfifEncoder<5> encoder {};
  ofstream       output  {};

  size_t start_frame {};
  size_t frames_count {};
  size_t views_count {};

  if (!parse_args_video(argc, argv, input_file_mask, output_file_name, quality, start_frame, predict)) {
    return 1;
  }

  for (size_t frame = start_frame; frame < get_mask_names_count(input_file_mask, '@'); frame++) {
    uint64_t frame_width         {};
    uint64_t frame_height        {};
    uint32_t frame_max_rgb_value {};

    size_t prev_cnt = ppm_data.size();

    if (mapPPMs(get_name_from_mask(input_file_mask, '@', frame).c_str(), frame_width, frame_height, frame_max_rgb_value, ppm_data) < 0) {
      cerr << "ERROR: IMAGE PROPERTIES MISMATCH\n";
      return -1;
    }

    size_t cnt = ppm_data.size() - prev_cnt;

    if (cnt == 0) {
      continue;
    }

    if (width && height && max_rgb_value && views_count) {
      if ((frame_width != width) || (frame_height != height) || (frame_max_rgb_value != max_rgb_value) || (cnt != views_count)) {
        cerr << "ERROR: FRAME DIMENSIONS MISMATCH" << endl;
        return -1;
      }
    }

    frames_count++;

    width         = frame_width;
    height        = frame_height;
    max_rgb_value = frame_max_rgb_value;
    views_count   = cnt;
  }

  if (!frames_count) {
    cerr << "ERROR: NO IMAGES LOADED\n";
    return 2;
  }

  if (create_directory(output_file_name)) {
    cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  output.open(output_file_name, ios::binary);
  if (!output) {
    cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  size_t side = sqrt(views_count);
  if (optind + 5 == argc) {
    for (size_t i { 0 }; i < 5; i++) {
      int tmp = atoi(argv[optind++]);
      if (tmp <= 0) {
        encoder.block_size = {side, side, side, side, side};
        break;
      }
      encoder.block_size[i] = tmp;
    }
  }
  else {
    encoder.block_size = {side, side, side, side, side};
  }

  encoder.img_dims[0] = width;
  encoder.img_dims[1] = height;
  encoder.img_dims[2] = side;
  encoder.img_dims[3] = side;
  encoder.img_dims[4] = frames_count;
  encoder.img_dims[5] = 1;
  encoder.color_depth = ceil(log2(max_rgb_value + 1));

  encoder.use_huffman    = false;
  encoder.use_prediction = predict;

  auto rgb_puller = [&](const std::array<size_t, 6> &pos) -> std::array<uint16_t, 3> {
    size_t img_index = pos[1] * width + pos[0];
    size_t img       = ((pos[5] * frames_count + pos[4]) * side + pos[3]) * side + pos[2];

    return ppm_data[img].get(img_index);
  };

  auto rgb_pusher = [&](const std::array<size_t, 6> &pos, const std::array<uint16_t, 3> &RGB) {
    size_t img_index = pos[1] * width + pos[0];
    size_t img       = ((pos[5] * frames_count + pos[4]) * side + pos[3]) * side + pos[2];

    ppm_data[img].put(img_index, RGB);
  };

  auto yuv_puller = [&](const std::array<size_t, 6> &pos) -> std::array<INPUTUNIT, 3> {
    std::array<uint16_t, 3> RGB = rgb_puller(pos);

    INPUTUNIT Y  = YCbCr::RGBToY(RGB[0], RGB[1], RGB[2]) - pow(2, encoder.color_depth - 1);
    INPUTUNIT Cb = YCbCr::RGBToCb(RGB[0], RGB[1], RGB[2]);
    INPUTUNIT Cr = YCbCr::RGBToCr(RGB[0], RGB[1], RGB[2]);

    return {Y, Cb, Cr};
  };

  auto yuv_pusher = [&](const std::array<size_t, 6> &pos, const std::array<INPUTUNIT, 3> &values) {
    INPUTUNIT Y  = values[0] + pow(2, encoder.color_depth - 1);
    INPUTUNIT Cb = values[1];
    INPUTUNIT Cr = values[2];

    uint16_t R = clamp<INPUTUNIT>(round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, pow(2, encoder.color_depth) - 1);
    uint16_t G = clamp<INPUTUNIT>(round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, pow(2, encoder.color_depth) - 1);
    uint16_t B = clamp<INPUTUNIT>(round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, pow(2, encoder.color_depth) - 1);

    rgb_pusher(pos, {R, G, B});
  };

  initEncoder(encoder);
  constructQuantizationTables(encoder, "DEFAULT", quality);
  writeHeader(encoder, output);
  outputScanCABAC_DIAGONAL(encoder, yuv_puller, yuv_pusher, output);

  return 0;
}
