/******************************************************************************\
* SOUBOR: lfif4d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "decompress.h"
#include "plenoppm.h"

#include <lfif_decoder.h>

#include <cmath>

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_file_name  {};
  const char *output_file_mask {};

  vector<PPM> ppm_data         {};

  uint64_t width               {};
  uint64_t height              {};
  uint64_t image_count         {};
  uint32_t max_rgb_value       {};

  ifstream        input        {};
  LfifDecoder<4> decoder       {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return 1;
  }

  input.open(input_file_name, ios::binary);
  if (!input) {
    cerr << "ERROR: CANNON OPEN " << input_file_name << " FOR READING\n";
    return 1;
  }

  if (readHeader(decoder, input)) {
    cerr << "ERROR: IMAGE HEADER INVALID\n";
    return 2;
  }

  width         = decoder.img_dims[0];
  height        = decoder.img_dims[1];
  image_count   = decoder.img_dims[2] * decoder.img_dims[3] * decoder.img_dims[4];
  max_rgb_value = pow(2, decoder.color_depth) - 1;

  ppm_data.resize(image_count);

  if (createPPMs(output_file_mask, width, height, max_rgb_value, ppm_data) < 0) {
    return 3;
  }

  initDecoder(decoder);

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

    INPUTUNIT Y  = YCbCr::RGBToY(R, G, B) - pow(2, decoder.color_depth - 1);
    INPUTUNIT Cb = YCbCr::RGBToCb(R, G, B);
    INPUTUNIT Cr = YCbCr::RGBToCr(R, G, B);

    return {Y, Cb, Cr};
  };

  auto pusher = [&](size_t index, const std::array<INPUTUNIT, 3> &values) {
    size_t img       = index / (width * height);
    size_t img_index = index % (width * height);

    INPUTUNIT Y  = values[0] + pow(2, decoder.color_depth - 1);
    INPUTUNIT Cb = values[1];
    INPUTUNIT Cr = values[2];

    uint16_t R = clamp<INPUTUNIT>(round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, pow(2, decoder.color_depth) - 1);
    uint16_t G = clamp<INPUTUNIT>(round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, pow(2, decoder.color_depth) - 1);
    uint16_t B = clamp<INPUTUNIT>(round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, pow(2, decoder.color_depth) - 1);

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

  if (decoder.use_huffman) {
    //decodeScanHuffman(decoder, input, outputF);
  }
  else {
    decodeScanCABAC(decoder, input, puller, pusher);
  }

  return 0;
}
