/******************************************************************************\
* SOUBOR: lfif3d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"
#include "plenoppm.h"

#include <lfif_encoder.h>

#include <cmath>

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_file_mask  {};
  const char *output_file_name {};
  float quality                {};

  bool use_huffman             {};
  bool predict                 {};
  bool shift                   {};

  vector<PPM> ppm_data         {};

  uint64_t width               {};
  uint64_t height              {};
  uint64_t image_count         {};
  uint32_t max_rgb_value       {};

  LfifEncoder<3> encoder {};
  ofstream       output  {};

  if (!parse_args(argc, argv, input_file_mask, output_file_name, quality, use_huffman, predict, shift)) {
    return 1;
  }

  if (mapPPMs(input_file_mask, width, height, max_rgb_value, ppm_data) < 0) {
    cerr << "ERROR: IMAGES PROPERTIES MISMATCH\n";
    return 2;
  }

  image_count = ppm_data.size();

  if (!image_count) {
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

  size_t side = sqrt(image_count);
  if (optind + 3 == argc) {
    for (size_t i { 0 }; i < 3; i++) {
      int tmp = atoi(argv[optind++]);
      if (tmp <= 0) {
        encoder.block_size = {side, side, side};
        break;
      }
      encoder.block_size[i] = tmp;
    }
  }
  else {
    encoder.block_size = {side, side, side};
  }

  encoder.img_dims[0] = width;
  encoder.img_dims[1] = height;
  encoder.img_dims[2] = side;
  encoder.img_dims[3] = side;
  encoder.color_depth = ceil(log2(max_rgb_value + 1));

  encoder.use_huffman    = use_huffman;
  encoder.use_prediction = predict;

  auto rgb_puller = [&](const std::array<size_t, 4> &pos) -> std::array<uint16_t, 3> {
    size_t img_index = pos[1] * width + pos[0];
    size_t img       = pos[3] * side + pos[2];

    return ppm_data[img].get(img_index);
  };

  auto rgb_pusher = [&](const std::array<size_t, 4> &pos, const std::array<uint16_t, 3> &RGB) {
    size_t img_index = pos[1] * width + pos[0];
    size_t img       = pos[3] * side + pos[2];

    ppm_data[img].put(img_index, RGB);
  };

  auto yuv_puller = [&](const std::array<size_t, 4> &pos) -> std::array<INPUTUNIT, 3> {
    std::array<uint16_t, 3> RGB = rgb_puller(pos);

    INPUTUNIT Y  = YCbCr::RGBToY(RGB[0], RGB[1], RGB[2]) - pow(2, encoder.color_depth - 1);
    INPUTUNIT Cb = YCbCr::RGBToCb(RGB[0], RGB[1], RGB[2]);
    INPUTUNIT Cr = YCbCr::RGBToCr(RGB[0], RGB[1], RGB[2]);

    return {Y, Cb, Cr};
  };

  auto yuv_pusher = [&](const std::array<size_t, 4> &pos, const std::array<INPUTUNIT, 3> &values) {
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

  if (use_huffman) {
    constructTraversalTables(encoder, "DEFAULT");
    huffmanScan(encoder, yuv_puller);
    constructHuffmanTables(encoder);
    writeHeader(encoder, output);
    outputScanHuffman_RUNLENGTH(encoder, yuv_puller, output);
  }
  else {
    writeHeader(encoder, output);
    outputScanCABAC_DIAGONAL(encoder, yuv_puller, yuv_pusher, output);
  }

  return 0;
}
