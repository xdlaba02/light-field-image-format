/******************************************************************************\
* SOUBOR: lfif2d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"
#include "plenoppm.h"

#include <lfif_encoder.h>
#include <colorspace.h>

#include <cmath>

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_file_mask  {};
  const char *output_file_name {};
  float quality                {};

  bool use_huffman             {};

  vector<uint8_t> rgb_data     {};

  uint64_t width               {};
  uint64_t height              {};
  uint64_t image_count         {};
  uint32_t max_rgb_value       {};

  LfifEncoder<2> encoder {};
  ofstream       output  {};

  if (!parse_args(argc, argv, input_file_mask, output_file_name, quality, use_huffman)) {
    return 1;
  }

  if (!checkPPMheaders(input_file_mask, width, height, max_rgb_value, image_count)) {
    return 2;
  }

  size_t input_size = width * height * image_count * 3;
  input_size *= (max_rgb_value > 255) ? 2 : 1;
  rgb_data.resize(input_size);

  if (!loadPPMs(input_file_mask, rgb_data.data())) {
    return 2;
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

  encoder.block_size = {8, 8};

  encoder.img_dims[0] = width;
  encoder.img_dims[1] = height;
  encoder.img_dims[2] = image_count;
  encoder.color_depth = ceil(log2(max_rgb_value + 1));

  encoder.use_huffman = use_huffman;

  std::cerr << encoder.block_size[0] << ' ' << encoder.block_size[1] << '\n';
  std::cerr << encoder.img_dims[0] << ' ' << encoder.img_dims[1] << ' ' << encoder.img_dims[2] << '\n';
  std::cerr << (int)encoder.color_depth << '\n';
  std::cerr << encoder.use_huffman << '\n';

  auto inputF0 = [&](size_t channel, size_t index) -> RGBUNIT {
    if (max_rgb_value < 256) {
      return reinterpret_cast<const uint8_t *>(rgb_data.data())[index * 3 + channel];
    }
    else {
      return reinterpret_cast<const uint16_t *>(rgb_data.data())[index * 3 + channel];
    }
  };

  auto inputF = [&](size_t index) -> INPUTTRIPLET {
    RGBUNIT R = inputF0(0, index);
    RGBUNIT G = inputF0(1, index);
    RGBUNIT B = inputF0(2, index);

    INPUTUNIT  Y = YCbCr::RGBToY(R, G, B) - (max_rgb_value + 1) / 2;
    INPUTUNIT Cb = YCbCr::RGBToCb(R, G, B);
    INPUTUNIT Cr = YCbCr::RGBToCr(R, G, B);

    return {Y, Cb, Cr};
  };

  initEncoder(encoder);
  constructQuantizationTables(encoder, "DEFAULT", quality);

  if (use_huffman) {
    //referenceScan(encoder, inputF);
    constructTraversalTables(encoder, "DEFAULT");
    huffmanScan(encoder, inputF);
    constructHuffmanTables(encoder);
    writeHeader(encoder, output);
    outputScanHuffman_RUNLENGTH(encoder, inputF, output);
  }
  else {
    writeHeader(encoder, output);
    outputScanCABAC_DIAGONAL(encoder, inputF, output);
  }

  return 0;
}
