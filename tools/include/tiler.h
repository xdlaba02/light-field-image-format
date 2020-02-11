/**
* @file tiler.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 5. 2. 2020
* @copyright 2019 Drahomír Dlabaja
*/

#ifndef TILER_H
#define TILER_H

#include <block.h>

#include <cmath>

#include <array>
#include <map>

inline void shift_pos(std::array<size_t, 4> &pos, const std::array<int64_t, 2> &shift, size_t width, size_t height, size_t side) {
  int64_t shift_x = (pos[2] - (side / 2)) * shift[0];
  int64_t shift_y = (pos[3] - (side / 2)) * shift[1];

  std::array<int64_t, 2> shifted_pos {};

  shifted_pos[0] = pos[0] + shift_x;
  shifted_pos[1] = pos[1] + shift_y;

  while (shifted_pos[0] < 0) {
    shifted_pos[0] += width;
  }

  while (shifted_pos[1] < 0) {
    shifted_pos[1] += height;
  }

  pos[0] = shifted_pos[0] % width;
  pos[1] = shifted_pos[1] % height;
}


template <typename F>
std::array<int64_t, 2> find_best_tile_params(F &&inputF, size_t width, size_t height, size_t side, std::array<int64_t, 2> min, std::array<int64_t, 2> max, size_t iterations, int64_t sample_rate) {
  std::array<int64_t,  2> best_param {};
  std::array<uint64_t, 2> best_error {};

  std::array<std::map<int64_t, uint64_t>, 2> errors {};

  auto get_error_horizontal = [&](const auto &shift_param) {
    auto it = errors[0].find(shift_param[0]);
    if (it != errors[0].end()) {
      return it->second;
    }

    uint64_t error {};
    iterate_dimensions<4>(std::array {width, height, side - 1, side}, [&](const auto &pos) {
      if (rand() < RAND_MAX / sample_rate) {
        std::array<uint16_t, 3> v0 = inputF(std::array {pos[0], pos[1], pos[2] + 0, pos[3]}, shift_param);
        std::array<uint16_t, 3> v1 = inputF(std::array {pos[0], pos[1], pos[2] + 1, pos[3]}, shift_param);

        error += std::abs(v0[0] - v1[0]);
        error += std::abs(v0[1] - v1[1]);
        error += std::abs(v0[2] - v1[2]);
      }
    });

    errors[0][shift_param[0]] = error;

    return error;
  };

  auto get_error_vertical = [&](const auto &shift_param) {
    auto it = errors[1].find(shift_param[1]);
    if (it != errors[1].end()) {
      return it->second;
    }

    uint64_t error {};
    iterate_dimensions<4>(std::array {width, height, side, side - 1}, [&](const auto &pos) {
      if (rand() < RAND_MAX / sample_rate) {
        std::array<uint16_t, 3> v0 = inputF(std::array {pos[0], pos[1], pos[2], pos[3] + 0}, shift_param);
        std::array<uint16_t, 3> v1 = inputF(std::array {pos[0], pos[1], pos[2], pos[3] + 1}, shift_param);

        error += std::abs(v0[0] - v1[0]);
        error += std::abs(v0[1] - v1[1]);
        error += std::abs(v0[2] - v1[2]);
      }
    });

    errors[1][shift_param[1]] = error;

    return error;
  };

  double golden_ratio = (sqrt(5.) + 1.) / 2.;

  best_error[0] = get_error_horizontal(std::array {0, 0});
  best_error[1] = get_error_vertical(std::array {0, 0});

  for (size_t i {}; i < iterations; i++) {
    std::array<int64_t,  2> param     {};
    std::array<uint64_t, 2> error_min {};
    std::array<uint64_t, 2> error_max {};

    for (size_t d {}; d < 2; d++) {
      param[d] = std::round(max[d] - (max[d] - min[d]) / golden_ratio);
    }

    error_min[0] = get_error_horizontal(param);
    error_min[1] = get_error_vertical(param);

    for (size_t d {}; d < 2; d++) {
      if (error_min[d] < best_error[d]) {
        best_error[d] = error_min[d];
        best_param[d] = param[d];
      }
    }

    for (size_t d {}; d < 2; d++) {
      param[d] = std::round(min[d] + (max[d] - min[d]) / golden_ratio);
    }

    error_max[0] = get_error_horizontal(param);
    error_max[1] = get_error_vertical(param);

    for (size_t d {}; d < 2; d++) {
      if (error_max[d] < best_error[d]) {
        best_error[d] = error_max[d];
        best_param[d] = param[d];
      }
    }

    for (size_t d {}; d < 2; d++) {
      if (error_min[d] < error_max[d]) {
        max[d] = std::round(min[d] + (max[d] - min[d]) / golden_ratio);
      }
      else {
        min[d] = std::round(max[d] - (max[d] - min[d]) / golden_ratio);
      }
    }

  }

  std::array<uint64_t, 2> error {};

  error[0] = get_error_horizontal(std::array {best_param[0], best_param[0]});
  error[1] = get_error_vertical(  std::array {best_param[0], best_param[0]});

  for (size_t d {}; d < 2; d++) {
    if (error[d] < best_error[d]) {
      best_error[d] = error[d];
      best_param[d] = best_param[0];
    }
  }

  error[0] = get_error_horizontal(std::array {best_param[1], best_param[1]});
  error[1] = get_error_vertical(  std::array {best_param[1], best_param[1]});

  for (size_t d {}; d < 2; d++) {
    if (error[d] < best_error[d]) {
      best_error[d] = error[d];
      best_param[d] = best_param[1];
    }
  }

  return best_param;
}

#endif
