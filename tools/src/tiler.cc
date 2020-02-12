/******************************************************************************\
* SOUBOR: tiler.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <tiler.h>

#include <getopt.h>

#include <cmath>

#include <vector>
#include <iostream>

int main(int argc, char *argv[]) {
  const char *input_file_mask  {};
  const char *output_file_mask {};

  std::vector<PPM> input       {};
  std::vector<PPM> output      {};

  uint64_t width               {};
  uint64_t height              {};
  uint32_t max_rgb_value       {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:o:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
          continue;
        }
        break;

      case 'o':
        if (!output_file_mask) {
          output_file_mask = optarg;
          continue;
        }
        break;

      default:
        break;
    }

    return 1;
  }

  if (!input_file_mask || !output_file_mask) {
    return 1;
  }

  if (mapPPMs(input_file_mask, width, height, max_rgb_value, input) < 0) {
    return 2;
  }

  if (!input.size()) {
    return 3;
  }

  size_t side {static_cast<size_t>(sqrt(input.size()))};

  auto rgb_puller = [&](const std::array<size_t, 4> &pos) -> std::array<uint16_t, 3> {
    size_t img_index = pos[1] * width + pos[0];
    size_t img       = pos[3] * side  + pos[2];

    return input[img].get(img_index);
  };

  auto rgb_pusher = [&](const std::array<size_t, 4> &pos, const std::array<uint16_t, 3> &RGB) {
    size_t img_index = pos[1] * width + pos[0];
    size_t img       = pos[3] * side  + pos[2];

    output[img].put(img_index, RGB);
  };

  std::array<int64_t, 2> param {};
  if (optind + 2 == argc) {
    for (size_t i { 0 }; i < 2; i++) {
      param[i] = atof(argv[optind++]);
    }
  }
  else {
    int64_t width_half  = (static_cast<int64_t>(width) * 2) / side;
    int64_t height_half = (static_cast<int64_t>(height) * 2) / side;
    param = find_best_shift_params(rgb_puller, {width, height, side, side}, {-width_half, -height_half}, {width_half, height_half}, 20, 8);
    std::cerr << param[0] << ", " << param[1] << "\n";
  }

  output.resize(input.size());

  if (createPPMs(output_file_mask, width, height, max_rgb_value, output) < 0) {
    return 4;
  }

  iterate_dimensions<4>(std::array {width, height, side, side}, [&](const auto &pos) {
    rgb_pusher(pos, rgb_puller(get_shifted_pos(pos, {width, height, side, side}, param)));
  });

  return 0;
}
