/******************************************************************************\
* SOUBOR: lfif2d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"
#include "plenoppm.h"

#include <colorspace.h>
#include <lfif_encoder.h>

#include <cmath>

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_file_mask  {};
  const char *output_file_name {};
  float quality                {};

  vector<uint8_t> rgb_data     {};

  uint64_t width               {};
  uint64_t height              {};
  uint64_t image_count         {};
  uint32_t color_depth         {};

  LfifEncoder<8, 2> encoder    {};
  ofstream          output     {};

  auto inputF0 = [&](size_t channel, size_t index) -> RGBUNIT {
    if (color_depth < 256) {
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

    INPUTUNIT  Y = YCbCr::RGBToY(R, G, B) - (color_depth + 1) / 2;
    INPUTUNIT Cb = YCbCr::RGBToCb(R, G, B);
    INPUTUNIT Cr = YCbCr::RGBToCr(R, G, B);

    return {Y, Cb, Cr};
  };

  if (!parse_args(argc, argv, input_file_mask, output_file_name, quality)) {
    return 1;
  }

  if (!checkPPMheaders(input_file_mask, width, height, color_depth, image_count)) {
    return 2;
  }

  size_t input_size = width * height * image_count * 3;
  input_size *= (color_depth > 255) ? 2 : 1;
  rgb_data.resize(input_size);

  if (!loadPPMs(input_file_mask, rgb_data.data())) {
    return 2;
  }

  size_t last_slash_pos = string(output_file_name).find_last_of('/');
  if (last_slash_pos != string::npos) {
    string command = "mkdir -p " + string(output_file_name).substr(0, last_slash_pos);
    system(command.c_str());
  }

  output.open(output_file_name);
  if (!output) {
    cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  encoder.img_dims[0] = width;
  encoder.img_dims[1] = height;
  encoder.img_dims[2] = image_count;
  encoder.max_rgb_value = color_depth;

  initEncoder(encoder);
  constructQuantizationTables(encoder, "DEFAULT", quality);
  referenceScan(encoder, inputF); //FIRST IMAGE SCAN
  constructTraversalTables(encoder, "DEFAULT");
  huffmanScan(encoder, inputF); //SECOND IMAGE SCAN
  constructHuffmanTables(encoder);
  writeHeader(encoder, output);
  outputScan(encoder, inputF, output); //THIRD IMAGE SCAN

  return 0;
}
