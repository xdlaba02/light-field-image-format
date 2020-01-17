/**
* @file dct.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions which performs FDCT and IDCT.
*/

#ifndef DCT_H
#define DCT_H

#include "lfiftypes.h"
#include "meta.h"

#include <cmath>
#include <array>

/**
* @brief Type used for the DCT computations.
*/
using DCTDATAUNIT = float;

/**
* @brief  Function which initializes the DCT matrix.
* @return The DCT matrix.
*/
DynamicBlock<DCTDATAUNIT, 2> init_coefs(size_t BS) {
  DynamicBlock<DCTDATAUNIT, 2> coefs(BS);

  for(size_t u = 0; u < BS; ++u) {
    for(size_t x = 0; x < BS; ++x) {
      coefs[u * BS + x] = cos(((2 * x + 1) * u * M_PI ) / (2 * BS)) * sqrt(2.0 / BS) * (u == 0 ? (1 / sqrt(2)) : 1);
    }
  }

  return coefs;
}

/**
* @brief Struct for the FDCT which wraps static parameters for partial specialization.
*/
template <size_t D>
struct fdct {

  /**
  * @brief Function which performs the FDCT.
  * @param input  callback function returning samples from a block. Signature is DCTDATAUNIT input(size_t index).
  * @param output callback function for writing DCT coefficients. Signature is DCTDATAUNIT &output(size_t index).
  */
  template <typename IF, typename OF>
  fdct(const size_t BS[D], IF &&input, OF &&output) {
    DynamicBlock<DCTDATAUNIT, D> tmp(BS);

    for (size_t slice = 0; slice < BS[D - 1]; slice++) {
      auto inputF = [&](size_t index) {
        return input(slice * get_stride<D - 1>(BS) + index);
      };

      auto outputF = [&](size_t index) -> auto & {
        return tmp[slice * get_stride<D - 1>(BS) + index];
      };

      fdct<D - 1>(BS, inputF, outputF);
    }

    for (size_t noodle = 0; noodle < get_stride<D - 1>(BS); noodle++) {
      auto inputF = [&](size_t index) {
        return tmp[index * get_stride<D - 1>(BS) + noodle];
      };

      auto outputF = [&](size_t index) -> auto & {
        return output(index * get_stride<D - 1>(BS) + noodle);
      };

      fdct<1>(&BS[D - 1], inputF, outputF);
    }
  }
};

/**
 * @brief The parital specialization for putting one dimensional fdct.
 * @see fdct<BS, D>
 */
template <>
struct fdct<1> {

  /**
   * @brief The parital specialization for putting one dimensional fdct.
   * @see fdct<BS, D>::fdct
   */
  template <typename IF, typename OF>
  fdct(const size_t BS[1], IF &&input, OF &&output) {
    static DynamicBlock<DCTDATAUNIT, 2> coefs = init_coefs(BS[0]);

    for (size_t u = 0; u < BS[0]; u++) {
      for (size_t x = 0; x < BS[0]; x++) {
        output(u) += input(x) * coefs[u * BS[0] + x];
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
  * @param input  callback function returning coefficients from decoded block. Signature is DCTDATAUNIT input(size_t index).
  * @param output callback function for writing output samples. Signature is DCTDATAUNIT &output(size_t index).
  */
  template <typename IF, typename OF>
  idct(const size_t BS[D], IF &&input, OF &&output) {
    DynamicBlock<DCTDATAUNIT, D> tmp(BS);

    for (size_t slice = 0; slice < BS[D - 1]; slice++) {
      auto inputF = [&](size_t index) {
        return input(slice * get_stride<D - 1>(BS) + index);
      };

      auto outputF = [&](size_t index) -> auto & {
        return tmp[slice * get_stride<D - 1>(BS) + index];
      };

      idct<D - 1>(BS, inputF, outputF);
    }

    for (size_t noodle = 0; noodle < get_stride<D - 1>(BS); noodle++) {
      auto inputF = [&](size_t index) {
        return tmp[index * get_stride<D - 1>(BS) + noodle];
      };

      auto outputF = [&](size_t index) -> auto & {
        return output(index * get_stride<D - 1>(BS) + noodle);
      };

      idct<1>(&BS[D - 1], inputF, outputF);
    }
  }
};

/**
 * @brief The parital specialization for putting one dimensional idct.
 * @see idct<BS, D>
 */
template <>
struct idct<1> {

  /**
   * @brief The parital specialization for putting one dimensional idct.
   * @see idct<BS, D>::idct
   */
  template <typename IF, typename OF>
  idct(const size_t BS[1], IF &&input, OF &&output) {
    static DynamicBlock<DCTDATAUNIT, 2> coefs = init_coefs(BS[0]);

    for (size_t x = 0; x < BS[0]; x++) {
      for (size_t u = 0; u < BS[0]; u++) {
        output(x) += input(u) * coefs[u * BS[0] + x];
      }
    }
  }
};

#endif
