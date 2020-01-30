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

  bool use_huffman             {};
  bool predict                 {};

  vector<PPM> ppm_data         {};

  uint64_t width               {};
  uint64_t height              {};
  uint64_t image_count         {};
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
      break;
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

  if (create_directory(output_file_name)) {
    cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  output.open(output_file_name, ios::binary);
  if (!output) {
    cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  if (optind + 5 == argc) {
    for (size_t i { 0 }; i < 5; i++) {
      int tmp = atoi(argv[optind++]);
      if (tmp <= 0) {
        size_t block_size = sqrt(views_count);
        encoder.block_size = {block_size, block_size, block_size, block_size, block_size};
        break;
      }
      encoder.block_size[i] = tmp;
    }
  }
  else {
    size_t block_size = sqrt(image_count);
    encoder.block_size = {block_size, block_size, block_size, block_size, block_size};
  }

  encoder.img_dims[0] = width;
  encoder.img_dims[1] = height;
  encoder.img_dims[2] = sqrt(views_count);
  encoder.img_dims[3] = sqrt(views_count);
  encoder.img_dims[4] = frames_count;
  encoder.img_dims[5] = 1;
  encoder.color_depth = ceil(log2(max_rgb_value + 1));

  encoder.use_huffman    = false;
  encoder.use_prediction = predict;

  auto puller = [&](size_t index) -> std::array<INPUTUNIT, 3> {
    size_t img       = index / (width * height);
    size_t img_index = index % (width * height);

    uint16_t R {};
    uint16_t G {};
    uint16_t B {};

    if (max_rgb_value > 255) {
      BigEndian<uint16_t> *ptr = static_cast<BigEndian<uint16_t> *>(ppm_data[img].data());
      R = ptr[img_index * 3 + 0];
      G = ptr[img_index * 3 + 1];
      B = ptr[img_index * 3 + 2];
    }
    else {
      BigEndian<uint8_t> *ptr = static_cast<BigEndian<uint8_t> *>(ppm_data[img].data());
      R = ptr[img_index * 3 + 0];
      G = ptr[img_index * 3 + 1];
      B = ptr[img_index * 3 + 2];
    }

    INPUTUNIT Y  = YCbCr::RGBToY(R, G, B) - pow(2, encoder.color_depth - 1);
    INPUTUNIT Cb = YCbCr::RGBToCb(R, G, B);
    INPUTUNIT Cr = YCbCr::RGBToCr(R, G, B);

    return {Y, Cb, Cr};
  };

  auto pusher = [&](size_t index, const std::array<INPUTUNIT, 3> &values) {
    size_t img       = index / (width * height);
    size_t img_index = index % (width * height);

    INPUTUNIT Y  = values[0] + pow(2, encoder.color_depth - 1);
    INPUTUNIT Cb = values[1];
    INPUTUNIT Cr = values[2];

    uint16_t R = clamp<INPUTUNIT>(round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, pow(2, encoder.color_depth) - 1);
    uint16_t G = clamp<INPUTUNIT>(round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, pow(2, encoder.color_depth) - 1);
    uint16_t B = clamp<INPUTUNIT>(round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, pow(2, encoder.color_depth) - 1);

    if (max_rgb_value > 255) {
      BigEndian<uint16_t> *ptr = static_cast<BigEndian<uint16_t> *>(ppm_data[img].data());
      ptr[img_index * 3 + 0] = R;
      ptr[img_index * 3 + 1] = G;
      ptr[img_index * 3 + 2] = B;
    }
    else {
      BigEndian<uint8_t> *ptr = static_cast<BigEndian<uint8_t> *>(ppm_data[img].data());
      ptr[img_index * 3 + 0] = R;
      ptr[img_index * 3 + 1] = G;
      ptr[img_index * 3 + 2] = B;
    }
  };

  initEncoder(encoder);
  constructQuantizationTables(encoder, "DEFAULT", quality);
  writeHeader(encoder, output);
  outputScanCABAC_DIAGONAL(encoder, puller, pusher, output);

  return 0;
}
