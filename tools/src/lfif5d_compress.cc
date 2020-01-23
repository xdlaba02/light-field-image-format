/******************************************************************************\
* SOUBOR: lfif5d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"
#include "file_mask.h"
#include "plenoppm.h"

#include <colorspace.h>
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

  vector<PPM> ppm_data         {};

  uint64_t width               {};
  uint64_t height              {};
  uint64_t views_count         {};
  uint64_t frames_count        {};
  uint32_t max_rgb_value       {};

  LfifEncoder<5> encoder  {};
  ofstream       output   {};

  if (!parse_args_video(argc, argv, input_file_mask, output_file_name, quality, start_frame)) {
    return 1;
  }

  for (size_t frame = start_frame; frame < get_mask_names_count(input_file_mask, '@'); frame++) {
    uint64_t frame_width         {};
    uint64_t frame_height        {};
    uint32_t frame_max_rgb_value {};

    size_t prev_cnt = ppm_data.size();

    if (mapPPMs(get_name_from_mask(input_file_mask, '@', frame).c_str(), frame_width, frame_height, frame_max_rgb_value, ppm_data) < 0) {
      unmapPPMs(ppm_data);
      break;
    }

    size_t cnt = ppm_data.size() - prev_cnt;

    if (cnt == 0) {
      continue;
    }

    if (width && height && max_rgb_value && views_count) {
      if ((frame_width != width) || (frame_height != height) || (frame_max_rgb_value != max_rgb_value) || (cnt != views_count)) {
        cerr << "ERROR: FRAME DIMENSIONS MISMATCH" << endl;
        unmapPPMs(ppm_data);
        return -1;
      }
    }

    frames_count++;

    width         = frame_width;
    height        = frame_height;
    max_rgb_value = frame_max_rgb_value;
    views_count   = cnt;
  }

  size_t last_slash_pos = string(output_file_name).find_last_of('/');
  if (last_slash_pos != string::npos) {
    string command = "mkdir -p " + string(output_file_name).substr(0, last_slash_pos);
    system(command.c_str());
  }

  output.open(output_file_name, ios::binary);
  if (!output) {
    cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  encoder.block_size = {8, 8, sqrt(views_count), sqrt(views_count), frames_count};

  encoder.img_dims[0] = width;
  encoder.img_dims[1] = height;
  encoder.img_dims[2] = sqrt(views_count);
  encoder.img_dims[3] = sqrt(views_count);
  encoder.img_dims[4] = frames_count;
  encoder.img_dims[5] = 1;
  encoder.color_depth = ceil(log2(max_rgb_value + 1));


  auto inputF = [&](size_t index) -> INPUTTRIPLET {
    size_t img       = index / (width * height);
    size_t img_index = index % (width * height);

    RGBUNIT R = ppm_data[img][img_index * 3 + 0];
    RGBUNIT G = ppm_data[img][img_index * 3 + 1];
    RGBUNIT B = ppm_data[img][img_index * 3 + 2];

    INPUTUNIT  Y = YCbCr::RGBToY(R, G, B) - (max_rgb_value + 1) / 2;
    INPUTUNIT Cb = YCbCr::RGBToCb(R, G, B);
    INPUTUNIT Cr = YCbCr::RGBToCr(R, G, B);

    return {Y, Cb, Cr};
  };

  initEncoder(encoder);
  constructQuantizationTables(encoder, "DEFAULT", quality);
  writeHeader(encoder, output);
  outputScanCABAC_DIAGONAL(encoder, inputF, output);

  return 0;
}
