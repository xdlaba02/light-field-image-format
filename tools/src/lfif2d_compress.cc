/******************************************************************************\
* SOUBOR: lfif4d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"
#include "plenoppm.h"

#include <lfif.h>
#include <lfif_decoder.h>
#include <ppm.h>

#include <cmath>

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

int main(int argc, char *argv[]) {
  const char *input_file_name  {};
  const char *output_file_name {};

  uint8_t distortion {};
  bool    predict    {};
  bool    do_shift   {};

  if (!parse_args(argc, argv, input_file_name, output_file_name, distortion, predict, do_shift)) {
    return 1;
  }

  PPM ppm_image {};

  if (ppm_image.mmapPPM(input_file_name) < 0) {
    std::cerr << "ERROR: CANNOT OPEN IMAGE\n";
    return 2;
  }

  uint8_t color_depth = ceil(log2(ppm_image.color_depth() + 1));

  std::array<uint64_t, 2> image_size { ppm_image.width(), ppm_image.height() };
  std::array<uint64_t, 2> block_size {8, 8};
  if (optind + 2 == argc) {
    for (size_t i = 0; i < 2; i++) {
      std::stringstream ss {};
      ss << argv[optind++];
      size_t tmp {};
      ss >> tmp;
      if (!ss) {
        block_size = {8, 8};
        break;
      }

      block_size[i] = tmp;
    }
  }

  auto puller = [&](const std::array<size_t, 2> &pos) -> std::array<uint16_t, 3> {
    return ppm_image.get(pos[1] * image_size[0] + pos[0]);
  };

  if (create_directory(output_file_name)) {
    std::cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  std::ofstream output_file {};
  output_file.open(output_file_name, std::ios::binary);
  if (!output_file) {
    std::cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  output_file << "LFIF-2D\n";

  LFIF<2> output {};
  output.create(output_file, image_size, block_size, color_depth, distortion, predict);

  encodeStreamDCT(puller, output_file);

  return 0;
}
