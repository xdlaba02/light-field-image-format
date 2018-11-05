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
ZigzagTable<D> constructZigzagTable(const QuantTable<D> &quant_table) {
  ZigzagTable<D>                    zigzag_table {};
  Block<pair<uint8_t, uint16_t>, D> srt          {};

  // DC koeficient musí být vždycky první
  srt[0] = {0, 0};

  for (uint64_t i = 1; i < constpow(8, D); i++) {
    srt[i] = {quant_table[i], i};
  }

  stable_sort(srt.begin(), srt.end());

  for (uint64_t i = 0; i < constpow(8, D); i++) {
    zigzag_table[srt[i].second] = i;
  }

  return zigzag_table;
}

#endif
