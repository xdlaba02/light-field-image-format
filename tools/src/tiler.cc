/******************************************************************************\
* SOUBOR: tiler.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <endian_t.h>

#include <getopt.h>

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

  auto puller = [&](size_t img, size_t index) -> std::array<uint16_t, 3> {
    uint16_t R {};
    uint16_t G {};
    uint16_t B {};

    if (max_rgb_value > 255) {
      BigEndian<uint16_t> *ptr = static_cast<BigEndian<uint16_t> *>(input[img].data());
      R = ptr[index * 3 + 0];
      G = ptr[index * 3 + 1];
      B = ptr[index * 3 + 2];
    }
    else {
      BigEndian<uint8_t> *ptr = static_cast<BigEndian<uint8_t> *>(input[img].data());
      R = ptr[index * 3 + 0];
      G = ptr[index * 3 + 1];
      B = ptr[index * 3 + 2];
    }

    return {R, G, B};
  };

  auto pusher = [&](size_t img, size_t index, const std::array<uint16_t, 3> &values) {
    uint16_t R = values[0];
    uint16_t G = values[1];
    uint16_t B = values[2];

    if (max_rgb_value > 255) {
      BigEndian<uint16_t> *ptr = static_cast<BigEndian<uint16_t> *>(output[img].data());
      ptr[index * 3 + 0] = R;
      ptr[index * 3 + 1] = G;
      ptr[index * 3 + 2] = B;
    }
    else {
      BigEndian<uint8_t> *ptr = static_cast<BigEndian<uint8_t> *>(output[img].data());
      ptr[index * 3 + 0] = R;
      ptr[index * 3 + 1] = G;
      ptr[index * 3 + 2] = B;
    }
  };

  auto inputF = [&](const auto &pos, const auto &shift) -> std::array<uint16_t, 3> {
    int64_t shift_x = (pos[2] - (side / 2)) * shift[0];
		int64_t shift_y = (pos[3] - (side / 2)) * shift[1];

    int64_t pos_x = pos[0] + shift_x;
    int64_t pos_y = pos[1] + shift_y;

    while (pos_x < 0) {
      pos_x += width;
    }

    while (pos_y < 0) {
      pos_y += height;
    }

    return puller(pos[3] * side + pos[2], (pos_y % height) * width + (pos_x % width));
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
    param = find_best_tile_params(inputF, width, height, side, {-width_half, -height_half}, {width_half, height_half}, 20, 4);
    std::cerr << param[0] << ", " << param[1] << "\n";
  }

  output.resize(input.size());

  if (createPPMs(output_file_mask, width, height, max_rgb_value, output) < 0) {
    return 4;
  }

  iterate_dimensions<4>(std::array {width, height, side, side}, [&](const auto &pos) {
    pusher(pos[3] * side + pos[2], pos[1] * width + pos[0], inputF(pos, param));
  });

  return 0;
}
