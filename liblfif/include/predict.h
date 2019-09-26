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
#include <cmath>


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

template <size_t BS, size_t D>
void project_neighbours_to_main_ref(Block<INPUTUNIT, BS * 2 + 1, D - 1> &main_ref, const int8_t direction[D], const INPUTUNIT *src, const size_t input_stride[D + 1]) {
  size_t main_ref_idx {};
  int64_t ref_offset {};

  for (size_t d = 0; d < D; d++) {
    if (direction[d] >= direction[main_ref_idx]) {
      main_ref_idx = d;
    }
  }

  for (size_t d { 0 }; d < D; d++) {
    if (d != main_ref_idx) {
      if (direction[d] > 0) {
        for (size_t i { 0 }; i < constpow(BS * 2 + 1, D - 1); i++) {
          int64_t src_idx {};

          for (size_t dd = 0; dd < D - 1; dd++) {
            int64_t idx = dd < d ? dd : dd + 1;
            src_idx += (i % constpow(BS * 2 + 1, dd + 1)) / constpow(BS * 2 + 1, dd) * input_stride[idx];
          }

          int64_t dst_idx {};
          std::array<int64_t, D> dst_pos {};

          //rotate index to direction d
          dst_idx = i % constpow(BS * 2 + 1, d) + i / constpow(BS * 2 + 1, d) * constpow(BS * 2 + 1, d + 1);

          bool overflow {};
          for (size_t dd { 0 }; dd < D; dd++) {
            dst_pos[dd] = dst_idx % constpow(BS * 2 + 1, dd + 1) / constpow(BS * 2 + 1, dd);

            if (dd != main_ref_idx) {
              if (direction[dd] >= 0) {
                dst_pos[dd] += BS;
              }
              if (direction[dd] <= 0){
                dst_pos[dd] += 1;
              }
            }

            if (dst_pos[dd] >= static_cast<int64_t>(BS * 2 + 1)) {
              overflow = true;
            }
          }

          if (dst_pos[main_ref_idx] > static_cast<int64_t>(BS)) {
            overflow = true;
          }

          if (overflow) {
            continue;
          }

          int64_t distance { dst_pos[main_ref_idx] };

          for (size_t dd { 0 }; dd < D; dd++) {
            dst_pos[dd] *= direction[main_ref_idx];
            dst_pos[dd] -= direction[dd] * distance;
            dst_pos[dd] /= direction[main_ref_idx];
          }

          for (size_t dd { 0 }; dd < D; dd++) {
            if (dst_pos[dd] < 0) {
              overflow = true;
            }
            else if (dst_pos[dd] >= static_cast<int64_t>(BS * 2 + 1)) {
              overflow = true;
            }
          }

          if (overflow) {
            continue;
          }

          dst_idx = 0;
          size_t pow {};
          for (size_t dd { 0 }; dd < D; dd++) {
            if (dd != main_ref_idx) {
              dst_idx += dst_pos[dd] * constpow(BS * 2 + 1, pow);
              pow++;
            }
          }

          // Tady by se taky mozna hodilo kontrolovat, jestli tam uz neni nejaka hodnota s mensi chybou
          main_ref[dst_idx] = src[src_idx];
        }
      }
    }
  }

  size_t idx {};
  for (size_t d { 0 }; d < D; d++) {
    if (d != main_ref_idx) {
      if (direction[d] >= 0) {
        ref_offset += constpow(BS * 2 + 1, idx) * BS;
      }
      if (direction[d] <= 0){
        ref_offset += constpow(BS * 2 + 1, idx);
      }
      idx++;
    }
  }

  // copy main samples to main reference
  for (size_t i { 0 }; i < constpow(BS * 2 + 1, D - 1); i++) {
    int64_t rot_idx {};
    int64_t src_idx {};
    size_t dst_idx {};

    rot_idx = i % constpow(BS * 2 + 1, main_ref_idx) + i / constpow(BS * 2 + 1, main_ref_idx) * constpow(BS * 2 + 1, main_ref_idx + 1);


    for (size_t dd = 0; dd < D; dd++) {
      src_idx += (rot_idx % constpow(BS * 2 + 1, dd + 1)) / constpow(BS * 2 + 1, dd) * input_stride[dd];
    }

    dst_idx = i + ref_offset;

    bool overflow {};
    for (size_t dd = 0; dd < D; dd++) {
      if (((dst_idx % constpow(BS * 2 + 1, dd + 1)) / constpow(BS * 2 + 1, dd)) < ((ref_offset % constpow(BS * 2 + 1, dd + 1)) / constpow(BS * 2 + 1, dd))) {
        overflow = true;
      }
    }

    if (overflow) {
      continue;
    }


    main_ref[dst_idx] = src[src_idx];
  }
}

template <size_t BS, size_t D>
void predict_from_main_ref(Block<INPUTUNIT, BS, D> &output, const int8_t direction[D], const Block<INPUTUNIT, BS * 2 + 1, D - 1> &main_ref) {
  int64_t main_ref_offset {};
  size_t  main_ref_idx {};

  for (size_t d = 0; d < D; d++) {
    if (direction[d] >= direction[main_ref_idx]) {
      main_ref_idx = d;
    }
  }

  // find offset for main neighbour to make space for projected samples
  size_t idx {};
  for (size_t d { 0 }; d < D; d++) {
    if (d != main_ref_idx) {
      if (direction[d] >= 0) {
        main_ref_offset += constpow(BS * 2 + 1, idx) * BS;
      }
      idx++;
    }
  }

  for (size_t i { 0 }; i < constpow(BS, D); i++) {
    std::array<int64_t, D> pos {};

    for (size_t d { 0 }; d < D; d++) {
      pos[d] = ((i % constpow(BS, d + 1) / constpow(BS, d)) + 1);
    }

    int64_t distance { pos[main_ref_idx] };

    for (size_t d { 0 }; d < D; d++) {
      pos[d] *= direction[main_ref_idx];
      pos[d] -= direction[d] * distance;
      pos[d] = std::floor(pos[d] / static_cast<double>(direction[main_ref_idx]) + 0.5);
    }

    int64_t dst_idx {};
    size_t pow {};
    for (size_t d { 0 }; d < D; d++) {
      if (d != main_ref_idx) {
        // hodnoty by se mohly interpolovat?
        dst_idx += pos[d] * constpow(BS * 2 + 1, pow);
        pow++;
      }
    }

    output[i] = main_ref[dst_idx + main_ref_offset];
  }
}

template <size_t BS, size_t D>
void predict_direction(Block<INPUTUNIT, BS, D> &output, const int8_t direction[D], const INPUTUNIT *src, const size_t input_stride[D + 1]) {
  Block<INPUTUNIT, BS * 2 + 1, D - 1> ref {};

  int64_t ptr_offset { 0 };
  size_t  main_ref   { 0 };

  // find which neighbouring block will be main
  for (size_t d = 0; d < D; d++) {
    if (direction[d] >= direction[main_ref]) {
      main_ref = d;
    }
  }

  if (direction[main_ref] <= 0) {
    return;
  }

  for (size_t d { 0 }; d < D; d++) {
    if (direction[d] > 0) {
      ptr_offset -= input_stride[d];
    }
  }

  project_neighbours_to_main_ref<BS, D>(ref, direction, &src[ptr_offset], input_stride);
  predict_from_main_ref<BS, D>(output, direction, ref);
}

#endif
