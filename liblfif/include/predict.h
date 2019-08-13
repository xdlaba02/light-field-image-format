/**
* @file predict.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 9. 8. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Prediction stuff.
*/

#ifndef PREDICT_H
#define PREDICT_H

#include "lfiftypes.h"

#include <cstdint>
#include <cassert>

template<size_t BS, size_t D, typename T>
void getSlice(const Block<T, BS, D> &block, Block<T, BS, D - 1> &slice, size_t direction, size_t index) {
  assert(direction < D);
  assert(index < BS);

  for (size_t i { 0 }; i < constpow(BS, D - 1); i++) {
    size_t directional_index = i % constpow(BS, direction) + i / constpow(BS, direction) * constpow(BS, direction + 1);
    slice[i] = block[directional_index + index * constpow(BS, direction)];
  }
}

template<size_t BS, size_t D, typename T>
void putSlice(Block<T, BS, D> &block, const Block<T, BS, D - 1> &slice, size_t direction, size_t index) {
  assert(direction < D);
  assert(index < BS);

  for (size_t i { 0 }; i < constpow(BS, D - 1); i++) {
    size_t directional_index = i % constpow(BS, direction) + i / constpow(BS, direction) * constpow(BS, direction + 1);
    block[directional_index + index * constpow(BS, direction)] = slice[i];
  }
}

template<size_t BS, size_t D>
void predict1D(Block<INPUTUNIT, BS, D> &predicted, const std::array<size_t, D> &block_dims, const INPUTUNIT *decoded, size_t direction) {
  assert(direction < D);
  for (size_t j { 0 }; j < BS; j++) {
    for (size_t i { 0 }; i < constpow(BS, D - 1); i++) {
      size_t dst_index = i % constpow(BS, direction) + i / constpow(BS, direction) * constpow(BS, direction + 1);
      block[dst_index + j * constpow(BS, direction)] = decoded[i];
    }
  }
}

dir = 0 2D
i = 0 => src_idx = decoded                                                      - 1
i = 1 => src_idx = decoded + (block_dims[0] * BS) * 1                           - 1
i = 2 => src_idx = decoded + (block_dims[0] * BS) * 2                           - 1
src_idx = decoded + (block_dims[0] * BS) * i                                    - 1;

dir = 1 2D
i = 0 => src_idx = decoded + 0                                                  - (block_dims[0] * BS);
i = 1 => src_idx = decoded + 1                                                  - (block_dims[0] * BS);
i = 2 => src_idx = decoded + 2                                                  - (block_dims[0] * BS);
src_idx = decoded + i                                                           - (block_dims[0] * BS);


dir = 0 3D
i = 0 => src_idx = decoded                                                      - 1
i = 1 => src_idx = decoded + (block_dims[0] * BS) * 1                           - 1;
i = 2 => src_idx = decoded + (block_dims[0] * BS) * 2                           - 1;
...
i = 8 => src_idx = decoded + (block_dims[0] * BS) * 8                           - 1;
src_idx = decoded + (block_dims[0] * BS) * i                                    - 1;

dir = 1 3D
i = 0 => src_idx = decoded + 0                                                  - (block_dims[0] * BS);
i = 1 => src_idx = decoded + 1                                                  - (block_dims[0] * BS);
i = 2 => src_idx = decoded + 2                                                  - (block_dims[0] * BS);
..
i = 8 => src_idx = decoded + 0 + (block_dims[0] * block_dims[1] * BS * BS) * 1  - (block_dims[0] * BS);
i = 9 => src_idx = decoded + 1 + (block_dims[0] * block_dims[1] * BS * BS) * 1  - (block_dims[0] * BS);
src_idx = decoded + (i % 8) + (block_dims[0] * block_dims[1] * BS * BS) * (i / 8)
                                                                                - (block_dims[0] * BS);
dir = 2 3D
i = 0 => src_idx = decoded + 0                                                  - (block_dims[0] * block_dims[1] * BS * BS);
i = 1 => src_idx = decoded + 1                                                  - (block_dims[0] * block_dims[1] * BS * BS);
i = 2 => src_idx = decoded + 2                                                  - (block_dims[0] * block_dims[1] * BS * BS);
...
i = 8 => src_idx = decoded + 0 + (block_dims[0] * BS) * 1                       - (block_dims[0] * block_dims[1] * BS * BS);
i = 9 => src_idx = decoded + 1 + (block_dims[0] * BS) * 1                       - (block_dims[0] * block_dims[1] * BS * BS);
src_idx = decoded + (i % 8) + (block_dims[0] * BS) * (i / 8)                    - (block_dims[0] * block_dims[1] * BS * BS);

dir = 0 4D
i = 0  => src_idx = decoded                                                     - 1;
i = 1  => src_idx = decoded + (block_dims[0] * BS) * 1                          - 1;
i = 2  => src_idx = decoded + (block_dims[0] * BS) * 2                          - 1;
...
i = 8  => src_idx = decoded + (block_dims[0] * BS) * 8                          - 1;
...
i = 64 => src_idx = decoded + (block_dims[0] * BS) * 64                         - 1;
src_idx = decoded + (block_dims[0] * BS) * i                                    - 1;

dir = 1 4D
i = 0 => src_idx = decoded + 0                                                  - (block_dims[0] * BS);
i = 1 => src_idx = decoded + 1                                                  - (block_dims[0] * BS);
i = 2 => src_idx = decoded + 2                                                  - (block_dims[0] * BS);
..
i = 8 => src_idx = decoded + 0 + (block_dims[0] * block_dims[1] * BS * BS) * 1  - (block_dims[0] * BS);
i = 9 => src_idx = decoded + 1 + (block_dims[0] * block_dims[1] * BS * BS) * 1  - (block_dims[0] * BS);
src_idx = decoded + (block_dims[0] * block_dims[1] * BS * BS) * (i / 8) + (i % 8)
                                                                                - (block_dims[0] * BS);

template<size_t BS, size_t D>
void predictDiag(Block<INPUTUNIT, BS, D> &predicted, const std::array<Block<INPUTUNIT, BS, D - 1> *, D> &neighbours) {
  for (size_t dir = 0; dir < D; dir++) {
    if (neighbours[dir]) {
      putSlice<BS, D, INPUTUNIT>(predicted, *neighbours[dir], dir, 0);
    }
  }

  for (size_t i = 0; i < constpow(BS, D); i++) {
    int64_t prev = i;

    for (size_t d = 0; d < D; d++) {
      prev -= constpow(BS, d);
    }

    if (i % constpow(BS, D - 1) > prev % constpow(BS, D - 1)) {
      predicted[i] = predicted[prev];
    }
  }
}

template<size_t BS, size_t D>
void predictDC(Block<INPUTUNIT, BS, D> &predicted, const Block<INPUTUNIT, BS, D - 1> &neighbour, size_t direction) {
  INPUTUNIT avg {};

  assert(direction < D);

  for (size_t i { 0 }; i < constpow(BS, D - 1); i++) {
    avg += neighbour[i];
  }

  avg /= constpow(BS, D - 1);

  for (size_t i { 0 }; i < constpow(BS, D); i++) {
    predicted[i] = avg;
  }
}

template<size_t BS, typename F>
void predictVert(const F &decoded, Block<INPUTUNIT, BS, 2> &predicted) {
  for (size_t y { 0 }; y < BS; y++) {
    for (size_t x { 0 }; x < BS; x++) {
      predicted[y * BS + x] = decoded(x + 1);
    }
  }
}

template<size_t BS, typename F>
void predictHorizontal(const F &decoded, Block<INPUTUNIT, BS, 2> &predicted) {
  for (size_t y { 0 }; y < BS; y++) {
    for (size_t x { 0 }; x < BS; x++) {
      predicted[y * BS + x] = decoded(y * (BS + 1));
    }
  }
}

#endif
