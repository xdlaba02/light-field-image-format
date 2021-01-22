/**
* @file dct.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions which performs FDCT and IDCT.
*/

#pragma once

#include "block.h"
#include "meta.h"

#include <cmath>

#include <array>

/**
* @brief Struct for the FDCT which wraps static parameters for partial specialization.
*/
template <size_t D>
struct fdct {

  /**
  * @brief Function which performs the FDCT.
  * @param input  callback function returning samples from a block. Signature is float input(size_t index).
  * @param output callback function for writing DCT coefficients. Signature is float &output(size_t index).
  */
  template <typename F>
  fdct(const std::array<size_t, D> &block_size, F &&block) {

    std::array<size_t, D - 1> subblock_size {};
    std::copy(std::begin(block_size), std::end(block_size) - 1, std::begin(subblock_size));

    const size_t stride = get_stride<D - 1>(block_size);

    for (size_t slice = 0; slice < block_size[D - 1]; slice++) {
      auto subblock = [&](size_t index) -> auto & {
        return block(slice * stride + index);
      };

      fdct<D - 1>(subblock_size, subblock);
    }

    for (size_t noodle = 0; noodle < stride; noodle++) {
      auto subblock = [&](size_t index) -> auto & {
        return block(index * stride + noodle);
      };

      fdct<1>({block_size[D - 1]}, subblock);
    }
  }
};

/**
 * @brief The parital specialization for putting one dimensional fdct.
 * @see fdct<block_size, D>
 */
template <>
struct fdct<1> {

  /**
   * @brief The parital specialization for putting one dimensional fdct.
   * @see fdct<block_size, D>::fdct
   */
  template <typename F>
  fdct(const std::array<size_t, 1> &block_size, F &&block) {
    DynamicBlock<float, 1> inputs({block_size[0]});

    const float c1 = sqrt(2.f / block_size[0]);
    const float c2 = 1.f / sqrt(2.f);
    const float c3 = 1.f / (2.f * block_size[0]);

    for (size_t x = 0; x < block_size[0]; x++) {
      inputs[x] = block(x) * c1;
      block(x) = 0.f;
    }

    for (size_t x = 0; x < block_size[0]; x++) {
      block(0) += inputs[x] * c2;
    }

    for (size_t u = 1; u < block_size[0]; u++) {
      for (size_t x = 0; x < block_size[0]; x++) {
        block(u) += inputs[x] * cos((2 * x + 1) * u * M_PI * c3);
      }
    }
  }
};

/**
* @brief Struct for the IDCT which wraps static parameters for partial specialization.
*/
template <size_t D>
struct idct {

  /**
  * @brief Function which performs the IDCT.
  * @param input  callback function returning coefficients from decoded block. Signature is float input(size_t index).
  * @param output callback function for writing output samples. Signature is float &output(size_t index).
  */
  template <typename F>
  idct(const std::array<size_t, D> &block_size, F &&block) {

    std::array<size_t, D - 1> subblock_size {};
    std::copy(std::begin(block_size), std::end(block_size) - 1, std::begin(subblock_size));

    for (size_t slice = 0; slice < block_size[D - 1]; slice++) {
      auto subblock = [&](size_t index) -> auto & {
        return block(slice * get_stride<D - 1>(block_size) + index);
      };

      idct<D - 1>(subblock_size, subblock);
    }

    for (size_t noodle = 0; noodle < get_stride<D - 1>(block_size); noodle++) {
      auto subblock = [&](size_t index) -> auto & {
        return block(index * get_stride<D - 1>(block_size) + noodle);
      };

      idct<1>({ block_size[D - 1] }, subblock);
    }
  }
};

/**
 * @brief The parital specialization for putting one dimensional idct.
 * @see idct<block_size, D>
 */
template <>
struct idct<1> {

  /**
   * @brief The parital specialization for putting one dimensional idct.
   * @see idct<block_size, D>::idct
   */
  template <typename F>
  idct(const std::array<size_t, 1> &block_size, F &&block) {
    DynamicBlock<float, 1> inputs({block_size[0]});

    float c1 = sqrt(2.f / block_size[0]);
    float c2 = 1.f / sqrt(2.f);
    float c3 = 1.f / (2.f * block_size[0]);

    for (size_t x = 0; x < block_size[0]; x++) {
      inputs[x] = block(x) * c1;
    }

    for (size_t x = 0; x < block_size[0]; x++) {
      block(x) = inputs[0] * c2;
    }

    for (size_t u = 1; u < block_size[0]; u++) {
      for (size_t x = 0; x < block_size[0]; x++) {
        block(x) += inputs[u] * cos((2 * x + 1) * u * M_PI * c3);
      }
    }
  }
};
