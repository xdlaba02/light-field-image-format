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

  auto puller = [&](size_t index, size_t channel) -> uint16_t {
    size_t img       = index / (width * height);
    size_t img_index = index % (width * height);

    if (max_rgb_value > 255) {
      BigEndian<uint16_t> *ptr = static_cast<BigEndian<uint16_t> *>(ppm_data[img].data());
      return ptr[img_index * 3 + channel];
    }
    else {
      BigEndian<uint8_t> *ptr = static_cast<BigEndian<uint8_t> *>(ppm_data[img].data());
      return ptr[img_index * 3 + channel];
    }
  };

  auto pusher = [&](size_t index, size_t channel, uint16_t val) {
    size_t img       = index / (width * height);
    size_t img_index = index % (width * height);

    if (max_rgb_value > 255) {
      BigEndian<uint16_t> *ptr = static_cast<BigEndian<uint16_t> *>(ppm_data[img].data());
      ptr[img_index * 3 + channel] = val;
    }
    else {
      BigEndian<uint8_t> *ptr = static_cast<BigEndian<uint8_t> *>(ppm_data[img].data());
      ptr[img_index * 3 + channel] = val;
    }
  };

  initDecoder(decoder);
  decodeScanCABAC(decoder, input, puller, pusher);

  return 0;
}
