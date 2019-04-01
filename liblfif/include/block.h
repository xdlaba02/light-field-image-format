/******************************************************************************\
* SOUBOR: block.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef BLOCK_H
#define BLOCK_H

#include "lfiftypes.h"

#include <cmath>

template<size_t D>
struct getBlock {
  template <typename IF, typename OF>
  getBlock(IF &&input, size_t block, const size_t dims[D], OF &&output) {
    size_t blocks_x = 1;
    size_t size_x   = 1;

    for (size_t i = 0; i < D - 1; i++) {
      blocks_x *= ceil(dims[i]/static_cast<double>(BLOCK_SIZE));
      size_x *= dims[i];
    }

    for (size_t pixel = 0; pixel < BLOCK_SIZE; pixel++) {
      size_t image_y = (block / blocks_x) * BLOCK_SIZE + pixel;

      if (image_y >= dims[D-1]) {
        image_y = dims[D-1] - 1;
      }

      auto inputF = [&](size_t image_index){
        return input(image_y * size_x + image_index);
      };

      auto outputF = [&](size_t pixel_index) -> auto & {
        return output(pixel * constpow(BLOCK_SIZE, D-1) + pixel_index);
      };

      getBlock<D-1>(inputF, block % blocks_x, dims, outputF);
    }
  }
};

template<>
struct getBlock<1> {
  template <typename IF, typename OF>
  getBlock(IF &&input, const size_t block, const size_t dims[1], OF &&output) {
    for (size_t pixel = 0; pixel < BLOCK_SIZE; pixel++) {
      size_t image = block * BLOCK_SIZE + pixel;

      if (image >= dims[0]) {
        image = dims[0] - 1;
      }

      output(pixel) = input(image);
    }
  }
};

template<size_t D>
struct putBlock {
  template <typename IF, typename OF>
  putBlock(IF &&input, size_t block, const size_t dims[D], OF &&output) {
    size_t blocks_x = 1;
    size_t size_x   = 1;

    for (size_t i = 0; i < D - 1; i++) {
      blocks_x *= ceil(dims[i]/static_cast<double>(BLOCK_SIZE));
      size_x *= dims[i];
    }

    for (size_t pixel = 0; pixel < BLOCK_SIZE; pixel++) {
      size_t image = (block / blocks_x) * BLOCK_SIZE + pixel;

      if (image >= dims[D-1]) {
        break;
      }

      auto inputF = [&](size_t pixel_index){
        return input(pixel * constpow(BLOCK_SIZE, D-1) + pixel_index);
      };

      auto outputF = [&](size_t image_index)-> auto & {
        return output(image * size_x + image_index);
      };

      putBlock<D-1>(inputF, block % blocks_x, dims, outputF);
    }
  }
};

template<>
struct putBlock<1> {
  template <typename IF, typename OF>
  putBlock(IF &&input, size_t block, const size_t dims[1], OF &&output) {
    for (size_t pixel = 0; pixel < BLOCK_SIZE; pixel++) {
      size_t image = block * BLOCK_SIZE + pixel;

      if (image >= dims[0]) {
        break;
      }

      output(image) = input(pixel);
    }
  }
};

#endif
