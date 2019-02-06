/******************************************************************************\
* SOUBOR: lfif3d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"
#include "zigzag.h"
#include "plenoppm.h"

#include <lfif_encoder.h>

#include <iostream>
/*
RGBData zigzagShiftRGB(const RGBData &rgb_data, uint64_t image_count) {
  RGBData zigzag_data(rgb_data.size());

  size_t image_size = rgb_data.size() / image_count;

  vector<size_t> zigzag_table = generateZigzagTable(sqrt(image_count));

  for (size_t i = 0; i < image_count; i++) {
    for (size_t j = 0; j < image_size; j++) {
      zigzag_data[zigzag_table[i] * image_size + j] = rgb_data[i * image_size + j];
    }
  }

  return zigzag_data;
}
*/
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
    cerr << "ERROR: UNSUPPORTED COLOR DEPTH. YET." << endl;
    return 2;
  }

  //rgb_data = zigzagShiftRGB(rgb_data, image_count);
  //TODO

  uint64_t img_dims[] {width, height, image_count};
  int errcode = LFIFCompress<3>(rgb_data, img_dims, 1, quality, output_file_name);

  switch (errcode) {
    case -1:
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << output_file_name << "\" FOR WRITITNG" << endl;
      return 3;
    break;

    default:
      return 0;
    break;
  }

  return 0;
}
