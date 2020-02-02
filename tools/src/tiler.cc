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

std::array<float, 2> find_best_tile_params(const std::vector<PPM> &ppm_data, std::array<float, 2> min, std::array<float, 2> max, size_t iterations) {
  assert(ppm_data.size());

  std::array<float, 2> param {};

  size_t width         {ppm_data[0].width()};
  size_t height        {ppm_data[0].height()};
  size_t max_rgb_value {ppm_data[0].color_depth()};
  size_t side          {static_cast<size_t>(sqrt(ppm_data.size()))};

  auto puller = [&](size_t img, size_t index) -> std::array<uint16_t, 3> {
    uint16_t R {};
    uint16_t G {};
    uint16_t B {};

    if (max_rgb_value > 255) {
      BigEndian<uint16_t> *ptr = static_cast<BigEndian<uint16_t> *>(ppm_data[img].data());
      R = ptr[index * 3 + 0];
      G = ptr[index * 3 + 1];
      B = ptr[index * 3 + 2];
    }
    else {
      BigEndian<uint8_t> *ptr = static_cast<BigEndian<uint8_t> *>(ppm_data[img].data());
      R = ptr[index * 3 + 0];
      G = ptr[index * 3 + 1];
      B = ptr[index * 3 + 2];
    }

    return {R, G, B};
  };

  auto inputF = [&](const std::array<size_t, 2> &img, const std::array<size_t, 2> &pos) -> std::array<uint16_t, 3> {
    float shift_x = (img[0] - (side / 2.f)) * param[0];
		float shift_y = (img[1] - (side / 2.f)) * param[1];

    int64_t floored_shift_x = std::floor(shift_x);
    int64_t floored_shift_y = std::floor(shift_y);

    int64_t pos_x = pos[0] + floored_shift_x;
    int64_t pos_y = pos[1] + floored_shift_y;

    while (pos_x < 0) {
      pos_x += width;
    }

    while (pos_y < 0) {
      pos_y += height;
    }

    size_t img_index = img[1] * side + img[0];

    std::array<uint16_t, 3> val_00 = puller(img_index, ((pos_y + 0) % height) * width + ((pos_x + 0) % width));
    std::array<uint16_t, 3> val_01 = puller(img_index, ((pos_y + 1) % height) * width + ((pos_x + 0) % width));
    std::array<uint16_t, 3> val_10 = puller(img_index, ((pos_y + 0) % height) * width + ((pos_x + 1) % width));
    std::array<uint16_t, 3> val_11 = puller(img_index, ((pos_y + 1) % height) * width + ((pos_x + 1) % width));

    float frac_x = shift_x - floored_shift_x;
    float frac_y = shift_y - floored_shift_y;

    std::array<double,   3> val_top    {};
    std::array<double,   3> val_bottom {};
    std::array<uint16_t, 3> val        {};

    val_top[0] = val_00[0] * (1.f - frac_x) + val_10[0] * frac_x;
    val_top[1] = val_00[1] * (1.f - frac_x) + val_10[1] * frac_x;
    val_top[2] = val_00[2] * (1.f - frac_x) + val_10[2] * frac_x;

    val_bottom[0] = val_01[0] * (1.f - frac_x) + val_11[0] * frac_x;
    val_bottom[1] = val_01[1] * (1.f - frac_x) + val_11[1] * frac_x;
    val_bottom[2] = val_01[2] * (1.f - frac_x) + val_11[2] * frac_x;

    val[0] = std::round(val_top[0] * (1.f - frac_y) + val_bottom[0] * frac_y);
    val[1] = std::round(val_top[1] * (1.f - frac_y) + val_bottom[1] * frac_y);
    val[2] = std::round(val_top[2] * (1.f - frac_y) + val_bottom[2] * frac_y);

    return val;
  };

  auto get_se_horizontal = [&]() {
    float squared_error {};

    iterate_dimensions<4>(std::array {width, height, side - 1, side}, [&](const auto &pos) {
      if (rand() < RAND_MAX / 4) {
        std::array<uint16_t, 3> v0 = inputF({pos[2] + 0, pos[3]}, {pos[0], pos[1]});
        std::array<uint16_t, 3> v1 = inputF({pos[2] + 1, pos[3]}, {pos[0], pos[1]});

        squared_error += (v0[0] - v1[0]) * (v0[0] - v1[0]);
        squared_error += (v0[1] - v1[1]) * (v0[1] - v1[1]);
        squared_error += (v0[2] - v1[2]) * (v0[2] - v1[2]);
      }
    });

    return squared_error;
  };

  auto get_se_vertical = [&]() {
    float squared_error {};

    iterate_dimensions<4>(std::array {width, height, side, side - 1}, [&](const auto &pos) {
      if (rand() < RAND_MAX / 4) {
        std::array<uint16_t, 3> v0 = inputF({pos[2], pos[3] + 0}, {pos[0], pos[1]});
        std::array<uint16_t, 3> v1 = inputF({pos[2], pos[3] + 1}, {pos[0], pos[1]});

        squared_error += (v0[0] - v1[0]) * (v0[0] - v1[0]);
        squared_error += (v0[1] - v1[1]) * (v0[1] - v1[1]);
        squared_error += (v0[2] - v1[2]) * (v0[2] - v1[2]);
      }
    });

    return squared_error;
  };

  float golden_ratio = (sqrt(5.f) + 1.f) / 2.f;

  for (size_t i {}; i < iterations; i++) {
    std::cerr << "iteration " << i << "\n";

    std::array<float, 2> lowest_se_min {};
    std::array<float, 2> lowest_se_max {};

    for (size_t d {}; d < 2; d++) {
      param[d] = max[d] - (max[d] - min[d]) / golden_ratio;
    }

    lowest_se_min[0] = get_se_horizontal();
    lowest_se_min[1] = get_se_vertical();

    for (size_t d {}; d < 2; d++) {
      param[d] = min[d] + (max[d] - min[d]) / golden_ratio;
    }

    lowest_se_max[0] = get_se_horizontal();
    lowest_se_max[1] = get_se_vertical();

    for (size_t d {}; d < 2; d++) {
      if (lowest_se_min[d] < lowest_se_max[d]) {
        max[d] = min[d] + ((max[d] - min[d]) / golden_ratio);
      }
      else {
        min[d] = max[d] - ((max[d] - min[d]) / golden_ratio);
      }
    }

    std::cerr << (min[0] + max[0]) / 2 << ", " << (min[1] + max[1]) / 2 << "\n";
  }

  for (size_t d {}; d < 2; d++) {
    param[d] = (min[d] + max[d]) / 2;
  }

  return param;
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

  output.resize(input.size());

  if (createPPMs(output_file_mask, width, height, max_rgb_value, output) < 0) {
    return 3;
  }

  std::array<float, 2> param {};

  if (optind + 2 == argc) {
    for (size_t i { 0 }; i < 2; i++) {
      param[i] = atof(argv[optind++]);
    }
  }
  else {
    param = find_best_tile_params(input, {-50, -50}, {50, 50}, 10);
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

  auto inputF = [&](const std::array<size_t, 2> &img, const std::array<size_t, 2> &pos) -> std::array<uint16_t, 3> {
    float shift_x = (img[0] - (side / 2.f)) * param[0];
		float shift_y = (img[1] - (side / 2.f)) * param[1];

    int64_t floored_shift_x = std::floor(shift_x);
    int64_t floored_shift_y = std::floor(shift_y);

    int64_t pos_x = pos[0] + floored_shift_x;
    int64_t pos_y = pos[1] + floored_shift_y;

    while (pos_x < 0) {
      pos_x += width;
    }

    while (pos_y < 0) {
      pos_y += height;
    }

    size_t img_index = img[1] * side + img[0];

    std::array<uint16_t, 3> val_00 = puller(img_index, ((pos_y + 0) % height) * width + ((pos_x + 0) % width));
    std::array<uint16_t, 3> val_01 = puller(img_index, ((pos_y + 1) % height) * width + ((pos_x + 0) % width));
    std::array<uint16_t, 3> val_10 = puller(img_index, ((pos_y + 0) % height) * width + ((pos_x + 1) % width));
    std::array<uint16_t, 3> val_11 = puller(img_index, ((pos_y + 1) % height) * width + ((pos_x + 1) % width));

    float frac_x = shift_x - floored_shift_x;
    float frac_y = shift_y - floored_shift_y;

    std::array<double,   3> val_top    {};
    std::array<double,   3> val_bottom {};
    std::array<uint16_t, 3> val        {};

    val_top[0] = val_00[0] * (1.f - frac_x) + val_10[0] * frac_x;
    val_top[1] = val_00[1] * (1.f - frac_x) + val_10[1] * frac_x;
    val_top[2] = val_00[2] * (1.f - frac_x) + val_10[2] * frac_x;

    val_bottom[0] = val_01[0] * (1.f - frac_x) + val_11[0] * frac_x;
    val_bottom[1] = val_01[1] * (1.f - frac_x) + val_11[1] * frac_x;
    val_bottom[2] = val_01[2] * (1.f - frac_x) + val_11[2] * frac_x;

    val[0] = std::round(val_top[0] * (1.f - frac_y) + val_bottom[0] * frac_y);
    val[1] = std::round(val_top[1] * (1.f - frac_y) + val_bottom[1] * frac_y);
    val[2] = std::round(val_top[2] * (1.f - frac_y) + val_bottom[2] * frac_y);

    return val;
  };

  iterate_dimensions<4>(std::array {width, height, side, side}, [&](const auto &pos) {
    pusher(pos[3] * side + pos[2], pos[1] * width + pos[0], inputF({pos[2], pos[3]}, {pos[0], pos[1]}));
  });

  return 0;
}
