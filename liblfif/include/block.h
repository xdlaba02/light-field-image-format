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
      blocks_x *= ceil(dims[i]/8.0);
      size_x *= dims[i];
    }

    for (size_t pixel = 0; pixel < 8; pixel++) {
      size_t image_y = (block / blocks_x) * 8 + pixel;

      if (image_y >= dims[D-1]) {
        image_y = dims[D-1] - 1;
      }

      auto inputF = [&](size_t image_index){
        return input(image_y * size_x * 3 + image_index);
      };

      auto outputF = [&](size_t pixel_index) -> RGBDataPixel &{
        return output(pixel * constpow(8, D-1) + pixel_index);
      };

      getBlock<D-1>(inputF, block % blocks_x, dims, outputF);
    }
  }
};

template<>
struct getBlock<1> {
  template <typename IF, typename OF>
  getBlock(IF &&input, const size_t block, const size_t dims[1], OF &&output) {
    for (size_t pixel = 0; pixel < 8; pixel++) {
      size_t image = block * 8 + pixel;

      if (image >= dims[0]) {
        image = dims[0] - 1;
      }

      output(pixel).r = input(image * 3 + 0);
      output(pixel).g = input(image * 3 + 1);
      output(pixel).b = input(image * 3 + 2);
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
      blocks_x *= ceil(dims[i]/8.0);
      size_x *= dims[i];
    }

    for (size_t pixel = 0; pixel < 8; pixel++) {
      size_t image = (block / blocks_x) * 8 + pixel;

      if (image >= dims[D-1]) {
        break;
      }

      auto inputF = [&](size_t pixel_index){
        return input(pixel * constpow(8, D-1) + pixel_index);
      };

      auto outputF = [&](size_t image_index)-> RGBDataUnit &{
        return output(image * size_x * 3 + image_index);
      };

      putBlock<D-1>(inputF, block % blocks_x, dims, outputF);
    }
  }
};

template<>
struct putBlock<1> {
  template <typename IF, typename OF>
  putBlock(IF &&input, size_t block, const size_t dims[1], OF &&output) {
    for (size_t pixel = 0; pixel < 8; pixel++) {
      size_t image = block * 8 + pixel;

      if (image >= dims[0]) {
        break;
      }

      output(image * 3 + 0) = input(pixel).r;
      output(image * 3 + 1) = input(pixel).g;
      output(image * 3 + 2) = input(pixel).b;
    }
  }
};

#endif
