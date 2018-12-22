/*******************************************************\
* SOUBOR: ppm2lfif2d.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 22. 12. 2018
\*******************************************************/

#include "compress.h"
#include "lfif_encoder.h"

#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_file_mask  {};
  const char *output_file_name {};
  uint8_t quality         {};

  if (!parse_args(argc, argv, input_file_mask, output_file_name, quality)) {
    return 1;
  }

  uint64_t width       {};
  uint64_t height      {};
  uint32_t color_depth {};
  uint64_t image_count {};

  RGBData rgb_data {};

  if (!loadPPMs(input_file_mask, rgb_data, width, height, color_depth, image_count)) {
    return 2;
  }

  if (color_depth != 255) {
    cerr << "UNSUPPORTED COLOR DEPTH" << endl;
    return 2;
  }

  uint64_t img_dims[] {width, height};
  if (!LFIFCompress<2>(rgb_data, img_dims, image_count, quality, output_file_name)) {
    return 3;
  }

  return 0;
}
