/**
* @file contexts.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 16. 7. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Context structures for various methods.
*/

#pragma once

#include "cabac.h"

#include <cstddef>

#include <vector>
#include <array>


template <size_t D>
struct ThresholdedDiagonalContexts {
  size_t threshold;

  std::vector<CABAC::ContextModel> coded_diag_flag_ctx;
  std::vector<CABAC::ContextModel> last_coded_diag_flag_ctx;
  std::vector<CABAC::ContextModel> significant_coef_flag_high_ctx;
  std::vector<CABAC::ContextModel> significant_coef_flag_low_ctx;
  std::vector<CABAC::ContextModel> coef_greater_one_ctx;
  std::vector<CABAC::ContextModel> coef_greater_two_ctx;
  std::vector<CABAC::ContextModel> coef_abs_level_ctx;

  ThresholdedDiagonalContexts(const std::array<size_t, D> &BS):
      threshold(num_diagonals<D>(BS) / 2),
      coded_diag_flag_ctx(num_diagonals<D>(BS)),
      last_coded_diag_flag_ctx(num_diagonals<D>(BS)),
      significant_coef_flag_high_ctx(D + 1),
      significant_coef_flag_low_ctx(D + 1),
      coef_greater_one_ctx(num_diagonals<D>(BS)),
      coef_greater_two_ctx(num_diagonals<D>(BS)),
      coef_abs_level_ctx(num_diagonals<D>(BS)) {}
};

template <class T>
inline constexpr T pow(const T base, const uint64_t exponent) {
    return (exponent == 0) ? 1 : (base * pow(base, exponent - 1));
}

template <size_t D>
struct PredictionModeContexts {
  std::array<CABAC::ContextModel, pow(5, D) + 3> prediction_ctx;
};
