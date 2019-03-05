/******************************************************************************\
* SOUBOR: zigzag.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef ZIGZAG_H
#define ZIGZAG_H

#include "constpow.h"

#include <cstdint>
#include <vector>
#include <array>
#include <numeric>

std::vector<size_t> generateZigzagTable2D(int64_t size);
std::vector<size_t> generateZigzagTable3D(int64_t size);
std::vector<size_t> generateZigzagTable4D(int64_t size);

template <size_t D>
void rotate(size_t *input, int64_t size) {
  std::vector<size_t> output(constpow(size, D));

  for (int64_t y = 0; y < size; y++) {
    for (int64_t x = 0; x < constpow(size, D-1); x++) {
      output[x * size + y] = input[y * constpow(size, D-1) + x];
    }
  }

  for (int64_t i = 0; i < constpow(size, D); i++) {
    input[i] = output[i];
  }
}

template <size_t D>
struct zigzagCore {
  template <typename F>
  zigzagCore(F &put, std::array<int64_t, D> dims, int64_t size, std::vector<size_t> &table, size_t DD) {
    while ((dims[D-1] < size) && (std::accumulate(dims.begin(), dims.end() - 1, 0) >= 0)) {
      auto put2 = [&](size_t i) {
        put(dims[D-1] * constpow(size, D-1) + i);
      };

      std::array<int64_t, D - 1> dims2 {};
      for (size_t i = 0; i < D - 1; i++) {
        dims2[i] = dims[i];
      }

      zigzagCore<D-1>(put2, dims2, size, table, DD);

      for (int64_t i = D - 2; i >= 0; i--) {
        if ((dims[i] > 0) || (i == 0)) {
          dims[i]--;
          break;
        }
      }

      dims[D-1]++;
    }

    for (int64_t i = 0; i < constpow(size, DD - D); i++) {
      rotate<D>(&table[i * constpow(size, D)], size);
    }
  }
};

template <>
struct zigzagCore<2> {
  template <typename F>
  zigzagCore(F &put, std::array<int64_t, 2> dims, int64_t size, std::vector<size_t> &table, size_t DD) {
    while ((dims[1] < size) && (dims[0] >= 0)) {
      put(dims[1] * size + dims[0]);
      dims[0]--;
      dims[1]++;
    }

    for (int64_t i = 0; i < constpow(size, DD - 2); i++) {
      rotate<2>(&table[i * constpow(size, 2)], size);
    }

  }
};

template <size_t D>
std::vector<size_t> generateZigzagTable(int64_t size) {
  std::vector<size_t> table(constpow(size, D));

  size_t index = 0;

  std::array<int64_t, D> dims {};

  while (std::accumulate(dims.begin(), dims.end(), 0) <= ((size - 1) * static_cast<int64_t>(D))) {
    auto put = [&](size_t i) {
      table[i] = index++;
    };

    zigzagCore<D>(put, dims, size, table, D);

    for (size_t i = 0; i < D; i++) {
      if ((dims[i] < size - 1) || (i == D - 1)) {
        dims[i]++;
        break;
      }
    }
  }

  return table;
}


#endif
