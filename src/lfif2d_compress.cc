/*******************************************************\
* SOUBOR: lfif2d_encoder.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 16. 12. 2018
\*******************************************************/

#include "lfif_encoder.h"
#include "ppm.h"

#include <getopt.h>
#include <iostream>
#include <algorithm>

using namespace std;

void print_usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <file-mask> -o <file> -q <quality>" << endl;
}

/*
bool loadMultiplePPMs(int filesc, char *filesv[], uint64_t &width, uint64_t &height, vector<uint8_t> &rgb_data) {
  uint64_t prev_width = 0;
  uint64_t prev_height = 0;

  for (int64_t i = 0; i < filesc; i++) {
    if (!loadPPM(filesv[i], width, height, rgb_data)) {
      cerr << "Unable to load image " << filesv[i] << "!" << endl;
      return false;
    }
    if (prev_width != 0 && prev_height != 0 && ((prev_width != width) || (prev_height != height))) {
      cerr << "Image sizes do not match!" << endl;
      return false;
    }

    prev_width = width;
    prev_height = height;
  }

  return true;
}*/

int main(int argc, char *argv[]) {
  char *arg_input_file_mask {nullptr};
  char *arg_output_file_name {nullptr};
  char *arg_quality {nullptr};

  /*** OPTIONS PARSING ***/
  char opt;
  while ((opt = getopt(argc, argv, "i:o:q:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!arg_input_file_mask) {
          arg_input_file_mask = optarg;
          continue;
        }
        break;

      case 'o':
        if (!arg_output_file_name) {
          arg_output_file_name = optarg;
          continue;
        }
        break;

      case 'q':
        if (!arg_quality) {
          arg_quality = optarg;
          continue;
        }
        break;

      default:
        break;
    }

    print_usage(argv[0]);
    return -1;
  }

  if ((!arg_input_file_mask) || (!arg_output_file_name) || (!arg_quality)) {
    print_usage(argv[0]);
    return -1;
  }

  string input_file_name  {arg_input_file_mask};
  string output_file_name {arg_output_file_name};
  uint8_t quality         {};

  int tmp_quality;
  tmp_quality = atoi(arg_quality);

  if ((tmp_quality < 1) || (tmp_quality > 100)) {
    print_usage(argv[0]);
    return -2;
  }
  else {
    quality = tmp_quality;
  }

  vector<uint64_t> mask_indexes {};

  for (uint64_t i = 0; arg_input_file_mask[i] != '\0'; i++) {
    if (arg_input_file_mask[i] == '#') {
      mask_indexes.push_back(i);
    }
  }

  uint64_t input_image_count = pow(10, mask_indexes.size());

  for (uint64_t image = 0; image < input_image_count; image++) {
    stringstream image_number {};

    image_number << setw(mask_indexes.size()) << setfill('0') << to_string(image);

    for (uint64_t index = 0; index < mask_indexes.size(); index++) {
      input_file_name[mask_indexes[index]] = image_number.str()[index];
    }

    uint64_t width;
    uint64_t height;
    vector<uint8_t> rgb_data;

    if (loadPPM(input_file_name, width, height, rgb_data)) {
      vector<float> Y_data  = convertRGB(rgb_data, RGBtoY);
      vector<float> Cb_data = convertRGB(rgb_data, RGBtoCb);
      vector<float> Cr_data = convertRGB(rgb_data, RGBtoCr);

      RGBtoLFIF<2>(output_filename, rgb_data, {width, height, count_x, count_y}, quality)
    }
  }

  /*
  vector<uint8_t> rgb_data {};
  uint64_t width           {};
  uint64_t height          {};
  uint64_t count_x         {};
  uint64_t count_y         {};

  if (!loadMultiplePPMs(argc - 6, argv + 6, width, height, rgb_data)) {
    return -2;
  }

  if (!) {
    return -3;
  }
  */

  return 0;
}
