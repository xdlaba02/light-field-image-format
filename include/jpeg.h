/*******************************************************\
* SOUBOR: jpeg.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 21. 10. 2018
\*******************************************************/

#ifndef JPEG_H
#define JPEG_H

#include "constmath.h"
#include "endian.h"

#include <cstdint>
#include <array>
#include <cmath>
#include <vector>
#include <tuple>
#include <algorithm>

using namespace std;

using Codeword = vector<bool>;

template<typename T, uint8_t D>
using Block = array<T, static_cast<uint16_t>(constpow(8, D))>;

template<uint8_t D>
using QuantTable = Block<uint8_t, D>;

template<uint8_t D>
using ZigzagTable = Block<uint16_t, D>;

struct RunLengthPair {
  uint8_t zeroes;
  int16_t amplitude;
};

template<uint8_t D>
ZigzagTable<D> constructZigzagTable() {
  ZigzagTable<D>                  zigzag_table {};
  Block<pair<float, uint16_t>, D> srt          {};

  for (uint64_t i = 0; i < constpow(8, D); i++) {
    uint64_t sum = 0;
    for (uint8_t j = 1; j <= D; j++) {
      uint8_t coord = (i % constpow(8, j)) / constpow(8, j-1);
      sum += coord * coord;
    }
    srt[i] = {sqrt(sum), i};
  }

  stable_sort(srt.begin(), srt.end());

  for (uint64_t i = 0; i < constpow(8, D); i++) {
    zigzag_table[srt[i].second] = i;
  }

  return zigzag_table;
}

#endif
