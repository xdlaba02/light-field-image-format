/**
* @file block.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for block extraction and insertion.
*/

#ifndef BLOCK_H
#define BLOCK_H

#include "lfiftypes.h"

#include <cmath>

/**
* @brief Struct for block extraction which wraps static parameters for partial specialization.
*/
template<size_t D>
struct getBlock {

  /**
   * @brief Function which extracts block.
   * @param input  The input callback function with signature T input(size_t index), where T is type of extracted sample.
   * @param block  Index of a wanted block in memory.
   * @param dims   The dimensions of an extracted volume.
   * @param output The output callback function with signature void output(size_t index, T value), where T is type of extracted sample.
   */
  template <typename IF, typename OF>
  getBlock(const size_t BS[D], IF &&input, const std::array<size_t, D> &pos_block, const std::array<size_t, D> &img_dims, OF &&output) {
    for (size_t block_y = 0; block_y < BS[D - 1]; block_y++) {
      size_t img_y = pos_block[D - 1] * BS[D - 1] + block_y;

      if (img_y >= img_dims[D - 1]) {
        img_y = img_dims[D - 1] - 1;
      }

      auto inputF = [&](const std::array<size_t, D - 1> &img_pos) {
        std::array<size_t, D> new_img_pos {};

        for (size_t i { 0 }; i < D - 1; i++) {
          new_img_pos[i] = img_pos[i];
        }
        new_img_pos[D - 1] = img_y;

        return input(new_img_pos);
      };

      auto outputF = [&](const std::array<size_t, D - 1> &block_pos, const auto &value) {
        std::array<size_t, D> new_block_pos {};

        for (size_t i { 0 }; i < D - 1; i++) {
          new_block_pos[i] = block_pos[i];
        }
        new_block_pos[D - 1] = block_y;

        output(new_block_pos, value);
      };

      std::array<size_t, D - 1> new_pos_block {};
      std::array<size_t, D - 1> new_img_dims  {};

      for (size_t i { 0 }; i < D - 1; i++) {
        new_pos_block[i] = pos_block[i];
         new_img_dims[i] =  img_dims[i];
      }

      getBlock<D - 1>(BS, inputF, new_pos_block, new_img_dims, outputF);
    }
  }
};

/**
 * @brief The parital specialization for getting one sample.
 * @see getBlock<BS, D>
 */
template<>
struct getBlock<0> {

  /**
   * @brief The parital specialization for getting one sample.
   * @see getBlock<BS, D>::getBlock
   */
  template <typename IF, typename OF>
  getBlock(const size_t *, IF &&input, const std::array<size_t, 0> &, const std::array<size_t, 0> &, OF &&output) {
    output({}, input({}));
  }
};

/**
* @brief Struct for block insertion which wraps static parameters for partial specialization.
*/
template<size_t D>
struct putBlock {

  /**
   * @brief Function which inserts block.
   * @param input  The input callback function with signature T input(size_t index), where T is type of inserted sample.
   * @param block  Index of a put block block in memory.
   * @param dims   The dimensions of an inserted volume to which block will be inserted.
   * @param output The output callback function with signature void output(size_t index, T value), where T is type of inserted sample.
   */
  template <typename IF, typename OF>
  putBlock(const size_t BS[D], IF &&input, const std::array<size_t, D> &pos_block, const std::array<size_t, D> &img_dims, OF &&output) {
    for (size_t block_y = 0; block_y < BS[D - 1]; block_y++) {
      size_t img_y = pos_block[D - 1] * BS[D - 1] + block_y;

      if (img_y >= img_dims[D - 1]) {
        break;
      }

      auto inputF = [&](const std::array<size_t, D - 1> &block_pos) {
        std::array<size_t, D> new_block_pos {};

        for (size_t i { 0 }; i < D - 1; i++) {
          new_block_pos[i] = block_pos[i];
        }
        new_block_pos[D - 1] = block_y;

        return input(new_block_pos);
      };

      auto outputF = [&](const std::array<size_t, D - 1> &img_pos, const auto &value) {
        std::array<size_t, D> new_img_pos {};

        for (size_t i { 0 }; i < D - 1; i++) {
          new_img_pos[i] = img_pos[i];
        }
        new_img_pos[D - 1] = img_y;

        return output(new_img_pos, value);
      };

      std::array<size_t, D - 1> new_pos_block {};
      std::array<size_t, D - 1> new_img_dims  {};

      for (size_t i { 0 }; i < D - 1; i++) {
        new_pos_block[i] = pos_block[i];
         new_img_dims[i] =  img_dims[i];
      }

      putBlock<D - 1>(BS, inputF, new_pos_block, new_img_dims, outputF);
    }
  }
};

/**
 * @brief The parital specialization for putting one sample.
 * @see putBlock<BS, D>
 */
template<>
struct putBlock<0> {

  /**
   * @brief The parital specialization for putting one sample.
   * @see putBlock<BS, D>::getBlock
   */
  template <typename IF, typename OF>
  putBlock(const size_t *, IF &&input, const std::array<size_t, 0> &, const std::array<size_t, 0> &, OF &&output) {
    output({}, input({}));
  }
};

#endif
