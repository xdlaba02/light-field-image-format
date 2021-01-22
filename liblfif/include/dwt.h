/**
* @file dwt.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 22. 1. 2021
* @copyright 2021 Drahomír Dlabaja
* @brief Functions which performs 5/3 FDWT and IDWT.
*/

#pragma once

#include "block.h"
#include "meta.h"

#include <cmath>

#include <array>

static inline int shift_right_and_round(int32_t a, int32_t b) {
   return (a + (1 << (b - 1))) >> b;
}

template <size_t D>
struct fdwt {
  template <typename F>
  fdwt(const std::array<size_t, D> &block_size, F &&block) {

    std::array<size_t, D - 1> subblock_size {};
    std::copy(std::begin(block_size), std::end(block_size) - 1, std::begin(subblock_size));

    const size_t stride = get_stride<D - 1>(block_size);

    for (size_t slice = 0; slice < block_size[D - 1]; slice++) {
      auto subblock = [&](size_t index) -> auto & {
        return block(slice * stride + index);
      };

      fdwt<D - 1>(subblock_size, subblock);
    }

    for (size_t noodle = 0; noodle < stride; noodle++) {
      auto subblock = [&](size_t index) -> auto & {
        return block(index * stride + noodle);
      };

      fdwt<1>({block_size[D - 1]}, subblock);
    }
  }
};

template <>
struct fdwt<1> {

  template <typename F>
  fdwt(const std::array<size_t, 1> &block_size, F &&block) {
    for (size_t x = 1; x < block_size[0]; x += 2) {
      int32_t left  = block(x - 1);
      int32_t right = x >= block_size[0] - 1 ? 0 : block(x + 1);

      block(x) -= shift_right_and_round(left + right, 1);
    }

    for (size_t x = 0; x < block_size[0]; x += 2) {
      int32_t left  = x == 0 ? 0 : block(x - 1);
      int32_t right = x >= block_size[0] - 1 ? 0 : block(x + 1);

      block(x) += shift_right_and_round(left + right, 2);
    }
  }
};

/**
* @brief Struct for the IDCT which wraps static parameters for partial specialization.
*/
template <size_t D>
struct idwt {

  template <typename F>
  idwt(const std::array<size_t, D> &block_size, F &&block) {

    std::array<size_t, D - 1> subblock_size {};
    std::copy(std::begin(block_size), std::end(block_size) - 1, std::begin(subblock_size));

    for (size_t slice = 0; slice < block_size[D - 1]; slice++) {
      auto subblock = [&](size_t index) -> auto & {
        return block(slice * get_stride<D - 1>(block_size) + index);
      };

      idwt<D - 1>(subblock_size, subblock);
    }

    for (size_t noodle = 0; noodle < get_stride<D - 1>(block_size); noodle++) {
      auto subblock = [&](size_t index) -> auto & {
        return block(index * get_stride<D - 1>(block_size) + noodle);
      };

      idwt<1>({ block_size[D - 1] }, subblock);
    }
  }
};

template <>
struct idwt<1> {

  template <typename F>
  idwt(const std::array<size_t, 1> &block_size, F &&block) {
    for (size_t x = 0; x < block_size[0]; x += 2) {
      int32_t left  = x == 0 ? 0 : block(x - 1);
      int32_t right = x >= block_size[0] - 1 ? 0 : block(x + 1);

      block(x) -= shift_right_and_round(left + right, 2);
    }

    for (size_t x = 1; x < block_size[0]; x += 2) {
      int32_t left  = block(x - 1);
      int32_t right = x >= block_size[0] - 1 ? 0 : block(x + 1);

      block(x) += shift_right_and_round(left + right, 1);
    }
  }
};
