/******************************************************************************\
* SOUBOR: tiler.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <endian_t.h>
#include <block.h>

#include <getopt.h>

#include <cassert>
#include <cmath>

#include <array>
#include <vector>
#include <iostream>

const int64_t SAMPLING_RATE = 4;

template <typename F>
std::array<int64_t, 2> find_best_tile_params(F &&inputF, size_t width, size_t height, size_t side, std::array<int64_t, 2> min, std::array<int64_t, 2> max, size_t iterations) {
  std::array<int64_t, 2> best_param         {};
  std::array<float,   2> best_squared_error {};


  auto get_error_horizontal = [&](const auto &shift_param) {
    float squared_error {};

    iterate_dimensions<4>(std::array {width, height, side - 1, side}, [&](const auto &pos) {
      if (rand() < RAND_MAX / SAMPLING_RATE) {
        std::array<uint16_t, 3> v0 = inputF(std::array {pos[0], pos[1], pos[2] + 0, pos[3]}, shift_param);
        std::array<uint16_t, 3> v1 = inputF(std::array {pos[0], pos[1], pos[2] + 1, pos[3]}, shift_param);

        squared_error += std::abs(v0[0] - v1[0]);
        squared_error += std::abs(v0[1] - v1[1]);
        squared_error += std::abs(v0[2] - v1[2]);
      }
    });

    return squared_error;
  };

  auto get_error_vertical = [&](const auto &shift_param) {
    float squared_error {};

    iterate_dimensions<4>(std::array {width, height, side, side - 1}, [&](const auto &pos) {
      if (rand() < RAND_MAX / SAMPLING_RATE) {
        std::array<uint16_t, 3> v0 = inputF(std::array {pos[0], pos[1], pos[2], pos[3] + 0}, shift_param);
        std::array<uint16_t, 3> v1 = inputF(std::array {pos[0], pos[1], pos[2], pos[3] + 1}, shift_param);

        squared_error += std::abs(v0[0] - v1[0]);
        squared_error += std::abs(v0[1] - v1[1]);
        squared_error += std::abs(v0[2] - v1[2]);
      }
    });

    return squared_error;
  };

  double golden_ratio = (sqrt(5.) + 1.) / 2.;

  size_t error_norm = (width * height * side * side * 3) / SAMPLING_RATE;

  best_squared_error[0] = get_error_horizontal(std::array {0, 0});
  best_squared_error[1] = get_error_vertical(std::array {0, 0});

  std::cerr << "shift params: [" << 0 << ", " << 0 << "]\n";
  std::cerr << "mean squared errors: [" << best_squared_error[0] / error_norm << ", " << best_squared_error[1] / error_norm << "]\n";


  for (size_t i {}; i < iterations; i++) {
    std::array<int64_t, 2> param             {};
    std::array<float,   2> squared_error_min {};
    std::array<float,   2> squared_error_max {};

    std::cerr << "iteration " << i << "\n";

    std::cerr << "min: [" << min[0] << ", " << min[1] << "]\n";
    std::cerr << "max: [" << max[0] << ", " << max[1] << "]\n";

    for (size_t d {}; d < 2; d++) {
      param[d] = std::round(max[d] - (max[d] - min[d]) / golden_ratio);
    }
    std::cerr << "shift params: [" << param[0] << ", " << param[1] << "]\n";

    squared_error_min[0] = get_error_horizontal(param);
    squared_error_min[1] = get_error_vertical(param);

    for (size_t d {}; d < 2; d++) {
      if (squared_error_min[d] < best_squared_error[d]) {
        best_squared_error[d] = squared_error_min[d];
        best_param[d]         = param[d];
      }
    }

    std::cerr << "mean squared errors: [" << squared_error_min[0] / error_norm << ", " << squared_error_min[1] / error_norm << "]\n";

    for (size_t d {}; d < 2; d++) {
      param[d] = std::round(min[d] + (max[d] - min[d]) / golden_ratio);
    }
    std::cerr << "shift params: [" << param[0] << ", " << param[1] << "]\n";

    squared_error_max[0] = get_error_horizontal(param);
    squared_error_max[1] = get_error_vertical(param);

    for (size_t d {}; d < 2; d++) {
      if (squared_error_max[d] < best_squared_error[d]) {
        best_squared_error[d] = squared_error_max[d];
        best_param[d]         = param[d];
      }
    }

    std::cerr << "mean squared errors: [" << squared_error_max[0] / error_norm << ", " << squared_error_max[1] / error_norm << "]\n";

    for (size_t d {}; d < 2; d++) {
      if (squared_error_min[d] < squared_error_max[d]) {
        max[d] = std::round(min[d] + (max[d] - min[d]) / golden_ratio);
      }
      else {
        min[d] = std::round(max[d] - (max[d] - min[d]) / golden_ratio);
      }
    }
  }

  return best_param;
}

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
    param = find_best_tile_params(inputF, width, height, side, {-50, -50}, {50, 50}, 10);
    std::cerr << param[0] << ", " << param[1] << "\n";
  }

  output.resize(input.size());

  if (createPPMs(output_file_mask, width, height, max_rgb_value, output) < 0) {
    return 4;
  }

  iterate_dimensions<4>(std::array {width, height, side, side}, [&](const auto &pos) {
    auto val = inputF(pos, param);
    pusher(pos[3] * side + pos[2], pos[1] * width + pos[0], val);
  });

  return 0;
}
