/******************************************************************************\
* SOUBOR: lfif3d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "decompress.h"
#include "plenoppm.h"

#include <lfif_decoder.h>
#include <colorspace.h>

#include <cmath>

#include <iostream>
#include <vector>

#ifdef BLOCK_SIZE
const size_t BS = BLOCK_SIZE;
#else
const size_t BS = 8;
#endif

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_file_name  {};
  const char *output_file_mask {};

  vector<uint8_t> rgb_data     {};

  LfifDecoder<BS, 3> *decoder  {};
  ifstream            input    {};

  uint16_t max_rgb_value       {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return 1;
  }

  input.open(input_file_name, ios::binary);
  if (!input) {
    cerr << "ERROR: CANNON OPEN " << input_file_name << " FOR READING\n";
    return 1;
  }

  decoder = new LfifDecoder<BS, 3>;

  if (readHeader(*decoder, input)) {
    cerr << "ERROR: IMAGE HEADER INVALID\n";
    return 2;
  }

  max_rgb_value = pow(2, decoder->color_depth) - 1;

  initDecoder(*decoder);

  size_t output_size = decoder->pixels_cnt * decoder->img_dims[3] * 3;
  output_size *= (decoder->color_depth > 8) ? 2 : 1;
  rgb_data.resize(output_size);

  auto outputF0 = [&](size_t channel, size_t index, RGBUNIT value) {
    if (decoder->color_depth > 8) {
      reinterpret_cast<uint16_t *>(rgb_data.data())[index * 3 + channel] = value;
    }
    else {
      reinterpret_cast<uint8_t *>(rgb_data.data())[index * 3 + channel] = value;
    }
  };

  auto outputF = [&](size_t index, const INPUTTRIPLET &triplet) {
    INPUTUNIT  Y = triplet[0] + ((max_rgb_value + 1) / 2);
    INPUTUNIT Cb = triplet[1];
    INPUTUNIT Cr = triplet[2];

    RGBUNIT R = clamp<INPUTUNIT>(round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, max_rgb_value);
    RGBUNIT G = clamp<INPUTUNIT>(round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, max_rgb_value);
    RGBUNIT B = clamp<INPUTUNIT>(round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, max_rgb_value);

    outputF0(0, index, R);
    outputF0(1, index, G);
    outputF0(2, index, B);
  };

  decodeScan(*decoder, input, outputF);

  if (!savePPMs(output_file_mask, rgb_data.data(), decoder->img_dims[0], decoder->img_dims[1], max_rgb_value, decoder->img_dims[2] * decoder->img_dims[3])) {
    return 3;
  }

  delete decoder;

  return 0;
}
