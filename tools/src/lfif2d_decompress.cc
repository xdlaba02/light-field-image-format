/******************************************************************************\
* SOUBOR: lfif4d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "decompress.h"
#include "plenoppm.h"

#include <ppm.h>
#include <lfif.h>
#include <lfif_decoder.h>

#include <cmath>

#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_stream_name {};
  const char *output_file_name  {};
  if (!parse_args(argc, argv, input_stream_name, output_file_name)) {
    return 1;
  }

  ifstream input_stream {};
  input_stream.open(input_stream_name, ios::binary);
  if (!input_stream) {
    cerr << "ERROR: CANNON OPEN " << input_stream_name << " FOR READING\n";
    return 1;
  }

  std::string magic_number {};
  input_stream >> magic_number;
  input_stream.ignore();

  if (magic_number != std::string("LFIF-2D")) {
    throw std::runtime_error("Magic number does not match!");
  }

  LFIF<2> input {};
  input.open(input_stream);

  PPM ppm_image {};
  if (ppm_image.createPPM(output_file_name, input.size[0], input.size[1], std::pow<float>(2, input.depth_bits) - 1) < 0) {
    return 3;
  }

  auto puller = [&](const std::array<size_t, 2> &pos) -> std::array<uint16_t, 3> {
    return ppm_image.get(pos[1] * input.size[0] + pos[0]);
  };

  auto pusher = [&](const std::array<size_t, 2> &pos, const std::array<uint16_t, 3> &RGB) {
    ppm_image.put(pos[1] * input.size[0] + pos[0], RGB);
  };

  decodeStreamDCT(input_stream, input, puller, pusher);

  return 0;
}
