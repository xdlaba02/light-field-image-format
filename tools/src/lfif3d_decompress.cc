/******************************************************************************\
* SOUBOR: lfif3d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "decompress.h"
#include "zigzag.h"

#include <iostream>

#include <lfif_decoder.h>

using namespace std;

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
  vector<size_t> vec(512);
  constructZigzagTable<3>(4, [&](size_t index) -> size_t & { return vec[index]; });

  for (size_t z = 0; z < 4; z++) {
    for (size_t y = 0; y < 4; y++) {
      for (size_t x = 0; x < 4; x++) {
        cerr << vec[z * 16 + y * 4 + x] << " ";
      }
      cerr << endl;
    }
    cerr << endl;
  }

  const char *input_file_name  {};
  const char *output_file_mask {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return 1;
  }

  uint64_t img_dims[3] {};
  uint64_t image_count {};

  RGBData rgb_data {};

  if (!LFIFDecompress<3>(input_file_name, rgb_data, img_dims, image_count)) {
    return 2;
  }

  rgb_data = zigzagDeshiftRGB(rgb_data, img_dims[2] * image_count);

  if (!savePPMs(rgb_data, img_dims[0], img_dims[1], 255, img_dims[2] * image_count, output_file_mask)) {
    return 3;
  }

  return 0;
}
