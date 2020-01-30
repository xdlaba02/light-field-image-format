/******************************************************************************\
* SOUBOR: lfif2d_compress.cc
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

  vector<PPM> ppm_data         {};

  uint64_t width               {};
  uint64_t height              {};
  uint64_t image_count         {};
  uint32_t max_rgb_value       {};

  LfifEncoder<2> encoder {};
  ofstream       output  {};

  if (!parse_args(argc, argv, input_file_mask, output_file_name, quality, use_huffman, predict)) {
    return 1;
  }

  if (mapPPMs(input_file_mask, width, height, max_rgb_value, ppm_data) < 0) {
    return 2;
  }

  image_count = ppm_data.size();

  if (create_directory(output_file_name)) {
    cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  output.open(output_file_name, ios::binary);
  if (!output) {
    cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  if (optind + 2 == argc) {
    for (size_t i { 0 }; i < 2; i++) {
      int tmp = atoi(argv[optind++]);
      if (tmp <= 0) {
        size_t block_size = sqrt(image_count);
        encoder.block_size = {block_size, block_size};
        break;
      }
      encoder.block_size[i] = tmp;
    }
  }
  else {
    size_t block_size = sqrt(image_count);
    encoder.block_size = {block_size, block_size};
  }

  encoder.img_dims[0] = width;
  encoder.img_dims[1] = height;
  encoder.img_dims[2] = image_count;
  encoder.color_depth = ceil(log2(max_rgb_value + 1));

  encoder.use_huffman = use_huffman;
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

  if (use_huffman) {
    constructTraversalTables(encoder, "DEFAULT");
    huffmanScan(encoder, puller);
    constructHuffmanTables(encoder);
    writeHeader(encoder, output);
    outputScanHuffman_RUNLENGTH(encoder, puller, output);
  }
  else {
    writeHeader(encoder, output);
    outputScanCABAC_DIAGONAL(encoder, puller, pusher, output);
  }

  return 0;
}
