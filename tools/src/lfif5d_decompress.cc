/******************************************************************************\
* SOUBOR: lfif5d_decompress.cc
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
  LfifDecoder<5> decoder       {};

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
  image_count   = decoder.img_dims[2] * decoder.img_dims[3] * decoder.img_dims[4] * decoder.img_dims[5];
  max_rgb_value = pow(2, decoder.color_depth) - 1;

  ppm_data.resize(image_count);

  if (createPPMs(output_file_mask, width, height, max_rgb_value, ppm_data) < 0) {
    return 3;
  }

  auto rgb_puller = [&](const std::array<size_t, 6> &pos) -> std::array<uint16_t, 3> {
    size_t img_index = pos[1] * width + pos[0];
    size_t img       = ((pos[5] * decoder.img_dims[4] + pos[4]) * decoder.img_dims[3] + pos[3]) * decoder.img_dims[2] + pos[2];

    return ppm_data[img].get(img_index);
  };

  auto rgb_pusher = [&](const std::array<size_t, 6> &pos, const std::array<uint16_t, 3> &RGB) {
    size_t img_index = pos[1] * width + pos[0];
    size_t img       = ((pos[5] * decoder.img_dims[4] + pos[4]) * decoder.img_dims[3] + pos[3]) * decoder.img_dims[2] + pos[2];

    ppm_data[img].put(img_index, RGB);
  };

  auto yuv_puller = [&](const std::array<size_t, 6> &pos) -> std::array<INPUTUNIT, 3> {
    std::array<uint16_t, 3> RGB = rgb_puller(pos);

    INPUTUNIT Y  = YCbCr::RGBToY(RGB[0], RGB[1], RGB[2]) - pow(2, decoder.color_depth - 1);
    INPUTUNIT Cb = YCbCr::RGBToCb(RGB[0], RGB[1], RGB[2]);
    INPUTUNIT Cr = YCbCr::RGBToCr(RGB[0], RGB[1], RGB[2]);

    return {Y, Cb, Cr};
  };

  auto yuv_pusher = [&](const std::array<size_t, 6> &pos, const std::array<INPUTUNIT, 3> &values) {
    INPUTUNIT Y  = values[0] + pow(2, decoder.color_depth - 1);
    INPUTUNIT Cb = values[1];
    INPUTUNIT Cr = values[2];

    uint16_t R = clamp<INPUTUNIT>(round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, pow(2, decoder.color_depth) - 1);
    uint16_t G = clamp<INPUTUNIT>(round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, pow(2, decoder.color_depth) - 1);
    uint16_t B = clamp<INPUTUNIT>(round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, pow(2, decoder.color_depth) - 1);

    rgb_pusher(pos, {R, G, B});
  };

  initDecoder(decoder);
  decodeScanCABAC(decoder, input, yuv_puller, yuv_pusher);

  return 0;
}
