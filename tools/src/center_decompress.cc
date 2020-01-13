/******************************************************************************\
* SOUBOR: center_decompress.cc
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
  const char *input_file_name   {};
  const char *output_file_mask  {};

  vector<uint8_t> rgb_data      {};
  vector<uint8_t> prediction    {};

  LfifDecoder<BS, 2> *decoder2D {};
  LfifDecoder<BS, 4> *decoder4D {};
  ifstream            input     {};

  uint16_t max_rgb_value     {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return 1;
  }

  input.open(input_file_name, ios::binary);
  if (!input) {
    cerr << "ERROR: CANNON OPEN " << input_file_name << " FOR READING\n";
    return 1;
  }

  decoder2D = new LfifDecoder<BS, 2>;
  decoder4D = new LfifDecoder<BS, 4>;

  if (readHeader(*decoder2D, input)) {
    cerr << "ERROR: IMAGE HEADER INVALID\n";
    return 2;
  }

  max_rgb_value = pow(2, decoder2D->color_depth) - 1;

  initDecoder(*decoder2D);

  size_t prediction_size = decoder2D->img_stride_unaligned[2] * decoder2D->img_dims[2] * 3;
  prediction_size *= (decoder2D->color_depth > 8) ? 2 : 1;
  prediction.resize(prediction_size);

  auto outputF02D = [&](size_t channel, size_t index, RGBUNIT value) {
    if (decoder2D->color_depth > 8) {
      reinterpret_cast<uint16_t *>(prediction.data())[index * 3 + channel] = value;
    }
    else {
      reinterpret_cast<uint8_t *>(prediction.data())[index * 3 + channel] = value;
    }
  };

  auto outputF2D = [&](size_t index, const INPUTTRIPLET &triplet) {
    INPUTUNIT  Y = triplet[0] + ((max_rgb_value + 1) / 2);
    INPUTUNIT Cb = triplet[1];
    INPUTUNIT Cr = triplet[2];

    RGBUNIT R = clamp<INPUTUNIT>(round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, max_rgb_value);
    RGBUNIT G = clamp<INPUTUNIT>(round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, max_rgb_value);
    RGBUNIT B = clamp<INPUTUNIT>(round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, max_rgb_value);

    outputF02D(0, index, R);
    outputF02D(1, index, G);
    outputF02D(2, index, B);
  };

  decodeScanCABAC(*decoder2D, input, outputF2D);

  if (readHeader(*decoder4D, input)) {
    cerr << "ERROR: IMAGE HEADER INVALID\n";
    return 2;
  }

  initDecoder(*decoder4D);

  size_t output_size = decoder4D->img_stride_unaligned[4] * decoder4D->img_dims[4] * 3;
  output_size *= (decoder4D->color_depth > 8) ? 2 : 1;
  rgb_data.resize(output_size);

  auto outputF0 = [&](size_t channel, size_t index, INPUTUNIT value) {
    if (max_rgb_value < 256) {
      reinterpret_cast<uint8_t *>(rgb_data.data())[index * 3 + channel] = clamp<INPUTUNIT>(round(value + reinterpret_cast<const uint8_t *>(prediction.data())[(index % decoder2D->img_stride_unaligned[2]) * 3 + channel] - max_rgb_value), 0, max_rgb_value);
    }
    else {
      reinterpret_cast<uint16_t *>(rgb_data.data())[index * 3 + channel] = clamp<INPUTUNIT>(round(value + reinterpret_cast<const uint16_t *>(prediction.data())[(index % decoder2D->img_stride_unaligned[2]) * 3 + channel] - max_rgb_value) , 0, max_rgb_value);
    }
  };

  auto outputF = [&](size_t index, const INPUTTRIPLET &triplet) {
    INPUTUNIT  Y = triplet[0] + max_rgb_value + 1;
    INPUTUNIT Cb = triplet[1];
    INPUTUNIT Cr = triplet[2];

    INPUTUNIT R = YCbCr::YCbCrToR(Y, Cb, Cr);
    INPUTUNIT G = YCbCr::YCbCrToG(Y, Cb, Cr);
    INPUTUNIT B = YCbCr::YCbCrToB(Y, Cb, Cr);

    outputF0(0, index, R);
    outputF0(1, index, G);
    outputF0(2, index, B);
  };

  decodeScanCABAC(*decoder4D, input, outputF);

  if (!savePPMs(output_file_mask, rgb_data.data(), decoder4D->img_dims[0], decoder4D->img_dims[1], max_rgb_value, decoder4D->img_dims[2] * decoder4D->img_dims[3] * decoder4D->img_dims[4])) {
    return 3;
  }

  delete decoder4D;
  delete decoder2D;

  return 0;
}
