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

/*
template <size_t D>
struct rotateF {
  template <typename IF, typename OF>
  rotateF(const IF &input, const OF &output, int64_t size) {
    for (int64_t y = 0; y < size; y++) {
      auto inputF = [&](size_t x) {
        return input(y * constpow(size, D-1) + x);
      };

      auto outputF = [&](size_t x) -> size_t & {
        return output(x * size + y);
      };

      rotateF<D-1>(inputF, outputF, size);
    }
  }
};

template <>
struct rotateF<1> {
  template <typename IF, typename OF>
  rotateF(const IF &input, const OF &output, int64_t size) {
    for (int64_t x = 0; x < size; x++) {
      output(x) = input(x);
    }
  }
};

template <size_t D>
void rotate(size_t *input, int64_t size) {
  std::vector<size_t> output(constpow(size, D));

  auto inputF = [&](size_t index) {
    return input[index];
  };

  auto outputF = [&](size_t index) -> size_t & {
    return output[index];
  };

  rotateF<D>(inputF, outputF, size);

  for (int64_t i = 0; i < constpow(size, D); i++) {
    input[i] = output[i];
  }
}

template <size_t D>
struct zigzagCore {
  template <typename F>
  zigzagCore(F &put, std::vector<size_t> &table, size_t rdepth, std::array<int64_t, D> dims, int64_t size) {
    while ((dims[D-1] < size) && (std::accumulate(dims.begin(), dims.end() - 1, 0) >= 0)) {
      std::array<int64_t, D - 1> dims2 {};
      for (size_t i = 0; i < D - 1; i++) {
        dims2[i] = dims[i];
      }

      zigzagCore<D-1>(put, table, rdepth+1, dims2, size);

      for (int64_t i = 0; i < constpow(size, rdepth); i++) {
        rotate<D>(&table[i * constpow(size, D)], size);
      }

      for (int64_t i = D - 2; i >= 0; i--) {
        if ((dims[i] > 0) || (i == 0)) {
          dims[i]--;
          break;
        }
      }

      dims[D-1]++;
    }
  }
};

template <>
struct zigzagCore<2> {
  template <typename F>
  zigzagCore(F &put, std::vector<size_t> &table, size_t rdepth, std::array<int64_t, 2> dims, int64_t size) {
    while ((dims[1] < size) && (dims[0] >= 0)) {
      put(dims[1] * size + dims[0]);
      dims[0]--;
      dims[1]++;
    }

    for (int64_t i = 0; i < constpow(size, rdepth); i++) {
      rotate<2>(&table[i * constpow(size, 2)], size);
    }
  }
};

template <size_t D>
std::vector<size_t> generateZigzagTable(int64_t size) {
  std::vector<size_t> table(constpow(size, D));

  size_t index = 0;

  std::array<int64_t, D> dims {};

  while (std::accumulate(dims.begin(), dims.end(), 0) <= static_cast<int64_t>((size - 1) * D)) {
    auto put = [&](size_t i) {
      table[i] = index++;
    };

    zigzagCore<D>(put, table, 0, dims, size);

    for (size_t i = 0; i < D; i++) {
      if (dims[i] < size - 1) {
        dims[i]++;
        break;
      }
      else if (i == D - 1) {
        dims[i]++;
        break;
      }
    }
  }

  return table;
}
*/

#endif
