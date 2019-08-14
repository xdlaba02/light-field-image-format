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
void predict_perpendicular(Block<INPUTUNIT, BS, D> &predicted, const size_t block_dims[D], const INPUTUNIT *decoded, size_t direction) {
  assert(direction < D);

  auto multDims = [&](size_t n) {
    size_t product { 1 };
    for (size_t i { 0 }; i < n; i++) {
      product *= block_dims[i] * BS;
    }
    return product;
  };

  for (size_t j { 0 }; j < BS; j++) {
    for (size_t i { 0 }; i < constpow(BS, D - 1); i++) {
      const INPUTUNIT *ptr { decoded };

      size_t pow_val { 0 };
      for (size_t d {0}; d < D; d++) {
        if (d == direction) {
          ptr -= multDims(d);
        }
        else {
          ptr += multDims(d) * ((i % constpow(BS, pow_val + 1)) / constpow(BS, pow_val));
          pow_val++;
        }
      }

      size_t dst_index = i % constpow(BS, direction) + i / constpow(BS, direction) * constpow(BS, direction + 1);
      predicted[dst_index + j * constpow(BS, direction)] = *ptr;
    }
  }
}

template<size_t BS, size_t D>
void predict_DC(Block<INPUTUNIT, BS, D> &predicted, const size_t block_dims[D], const INPUTUNIT *decoded) {
  INPUTUNIT avg { 0 };

  auto multDims = [&](size_t n) {
    size_t product { 1 };
    for (size_t i { 0 }; i < n; i++) {
      product *= block_dims[i] * BS;
    }
    return product;
  };

  for (size_t dir { 0 }; dir < D; dir++) {
    for (size_t i { 0 }; i < constpow(BS, D - 1); i++) {
      const INPUTUNIT *ptr { decoded };

      size_t pow_val { 0 };
      for (size_t d {0}; d < D; d++) {
        if (d == dir) {
          ptr -= multDims(d);
        }
        else {
          ptr += multDims(d) * ((i % constpow(BS, pow_val + 1)) / constpow(BS, pow_val));
          pow_val++;
        }
      }

      avg += *ptr;
    }
  }

  avg /= constpow(BS, D - 1) * D;

  for (size_t i { 0 }; i < constpow(BS, D); i++) {
    predicted[i] = avg;
  }
}
 /*
template<size_t BS, size_t D>
void predict_diagonal(Block<INPUTUNIT, BS, D> &predicted, const size_t block_dims[D], const INPUTUNIT *decoded) {

  auto multDims = [&](size_t n) {
    size_t product { 1 };
    for (size_t i { 0 }; i < n; i++) {
      product *= block_dims[i] * BS;
    }
    return product;
  };

  for (size_t i = 0; i < constpow(BS, D); i++) {
    INPUTUNIT *ptr { decoded };

    for (size_t d = 0; d < D; d++) {
      ptr += (i % constpow(BS, d + 1)) / constpow(BS, d) * multDims(d);
    }

    bool run { true };
    while (run) {
      for (size_t d = 0; d < D; d++) {
        ptr -= multDims(d);
        if ((ptr % constpow(BS, d + 1) / constpow(BS, d)) < 0) {
          run = false;
        }
      }
    }

    predicted[i] = *ptr;
  }
}*/

/*
dir = 0 2D
i = 0 => src_idx = decoded                                                      - 1
i = 1 => src_idx = decoded + (block_dims[0] * BS) * 1                           - 1
i = 2 => src_idx = decoded + (block_dims[0] * BS) * 2                           - 1
src_idx = decoded + (block_dims[0] * BS) * i                                    - 1;

dir = 1 2D
i = 0 => src_idx = decoded + 0                                                  - (block_dims[0] * BS);
i = 1 => src_idx = decoded + 1                                                  - (block_dims[0] * BS);
i = 2 => src_idx = decoded + 2                                                  - (block_dims[0] * BS);
src_idx = decoded + (i % 8) + (block_dims[0] * block_dims[1] * BS * BS) * (i / 8)
                                                                                - (block_dims[0] * BS);


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
...
src_idx = decoded + (i % 8) + (block_dims[0] * block_dims[1] * BS * BS) * (i / 8)
                                                                                - (block_dims[0] * BS);

dir = 2 4D
i =  0 => src_idx = decoded + 0                                                  - (block_dims[0] * block_dims[1] * BS * BS);
i =  1 => src_idx = decoded + 1                                                  - (block_dims[0] * block_dims[1] * BS * BS);
i =  2 => src_idx = decoded + 2                                                  - (block_dims[0] * block_dims[1] * BS * BS);
...
i =  8 => src_idx = decoded + 0 + (block_dims[0] * BS) * 1                       - (block_dims[0] * block_dims[1] * BS * BS);
i =  9 => src_idx = decoded + 1 + (block_dims[0] * BS) * 1                       - (block_dims[0] * block_dims[1] * BS * BS);
...
i = 64 => src_idx = decoded + 0 + (block_dims[0] * BS) * 1                       - (block_dims[0] * block_dims[1] * BS * BS);
src_idx = decoded + (i % 8) + (block_dims[0] * BS) * ((i % 64) / 8) + (block_dims[0] * block_dims[1] * block_dims[2] * BS * BS * BS) * (i / 64)                    - (block_dims[0] * block_dims[1] * BS * BS);
*/

#endif
