/******************************************************************************\
* SOUBOR: lfif3d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "decompress.h"
#include "zigzag.h"
#include "lfifppm.h"

#include <lfif_decoder.h>

#include <iostream>

RGBData zigzagDeshiftRGB(const RGBData &rgb_data, uint64_t image_count) {
  RGBData zigzag_data(rgb_data.size());

  size_t image_size = rgb_data.size() / image_count;

  vector<size_t> zigzag_table = generateZigzagTable(sqrt(image_count));

  for (size_t i = 0; i < image_count; i++) {
    for (size_t j = 0; j < image_size; j++) {
      zigzag_data[i * image_size + j] = rgb_data[zigzag_table[i] * image_size + j];
    }
  }

  return zigzag_data;
}

int main(int argc, char *argv[]) {
  const char *input_file_name  {};
  const char *output_file_mask {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return 1;
  }

  uint64_t img_dims[3] {};
  uint64_t image_count {};

  RGBData rgb_data {};

  int errcode = LFIFDecompress<3>(input_file_name, rgb_data, img_dims, image_count);

  switch (errcode) {
    case -1:
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << input_file_name << "\" FOR READING" << endl;
      return 2;
    break;

    case -2:
      cerr << "ERROR: MAGIC NUMBER MISMATCH" << endl;
      return 2;
    break;

    default:
    break;
  }

  rgb_data = zigzagDeshiftRGB(rgb_data, img_dims[2] * image_count);

  if (!savePPMs(rgb_data, img_dims[0], img_dims[1], 255, img_dims[2] * image_count, output_file_mask)) {
    return 3;
  }

  return 0;
}
