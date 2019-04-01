/******************************************************************************\
* SOUBOR: zigzag.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef ZIGZAG_H
#define ZIGZAG_H

#include "lfiftypes.h"

#include <cstdint>
#include <vector>
#include <array>
#include <numeric>

std::vector<size_t> generateZigzagTable2D(int64_t size);
std::vector<size_t> generateZigzagTable3D(int64_t size);
std::vector<size_t> generateZigzagTable4D(int64_t size);

template <size_t D>
void rotate(size_t *table) {
  Block<size_t, D> tmp {};

  for (size_t y = 0; y < BLOCK_SIZE; y++) {
    for (size_t x = 0; x < constpow(BLOCK_SIZE, D-1); x++) {
      tmp[x * BLOCK_SIZE + y] = table[y * constpow(BLOCK_SIZE, D-1) + x];
    }
  }

  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    table[i] = tmp[i];
  }
}

template <size_t D>
struct zigzagCore {
  template <typename F>
  zigzagCore(F &put, std::array<size_t, D> dims, size_t *table, size_t depth) {
    while ((dims[D-1] < BLOCK_SIZE) && (std::accumulate(dims.begin(), dims.end() - 1, 0) >= 0)) {
      std::array<size_t, D - 1> dims_new {};
      for (size_t i = 0; i < D - 1; i++) {
        dims_new[i] = dims[i];
      }

      auto put_new = [&](size_t i) {
        put(dims[D-1] * constpow(BLOCK_SIZE, D-1) + i);
      };

      zigzagCore<D-1>(put_new, dims_new, table, depth + 1);

      for (int64_t i = D - 2; i >= 0; i--) {
        if ((dims[i] > 0) || (i == 0)) {
          dims[i]--;
          break;
        }
      }

      dims[D-1]++;
    }

    for (size_t i = 0; i < constpow(BLOCK_SIZE, depth); i++) {
      rotate<D>(&table[i * constpow(BLOCK_SIZE, D)]);
    }
  }
};

template <>
struct zigzagCore<1> {
  template <typename F>
  zigzagCore(F &put, std::array<size_t, 1> dims, size_t *, size_t) {
    put(dims[0]);
  }
};

template <size_t D>
Block<size_t, D> generateZigzagTable() {
  Block<size_t, D> table {};

  size_t index = 0;

  std::array<size_t, D> dims {};

  while (std::accumulate(dims.begin(), dims.end(), size_t{} ) <= ((BLOCK_SIZE - 1) * D)) {
    auto put = [&](size_t i) {
      table[i] = index++;
    };

    zigzagCore<D>(put, dims, table.data(), 0);

    for (size_t i = 0; i < D; i++) {
      if ((dims[i] < BLOCK_SIZE - 1) || (i == D - 1)) {
        dims[i]++;
        break;
      }
    }
  }

  return table;
}


#endif
