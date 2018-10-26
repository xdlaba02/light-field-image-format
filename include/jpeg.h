/*******************************************************\
* SOUBOR: jpeg.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 21. 10. 2018
\*******************************************************/

#ifndef JPEG_H
#define JPEG_H

#include <cstdint>
#include <array>
#include <cmath>
#include <vector>
#include <tuple>

using namespace std;

using Codeword = vector<bool>;

template<class T>
inline constexpr T pow(const T base, const unsigned exponent) {
    return (exponent == 0) ? 1 : (base * pow(base, exponent - 1));
}

template<typename T, uint8_t D>
using Block = array<T, static_cast<uint16_t>(pow(8, D))>;

template<uint8_t D>
using QuantTable = Block<uint8_t, D>;

template<uint8_t D>
using ZigzagTable = Block<uint16_t, D>;

struct RunLengthPair {
  uint8_t zeroes;
  int16_t amplitude;
};

template<uint8_t D>
void constructZigzagTable(const QuantTable<D> &quant_table, ZigzagTable<D> &zigzag_table) {
  Block<pair<uint8_t, uint16_t>, D> srt {};

  for (uint16_t i = 0; i < pow(8, D); i++) {
    srt[i] = {quant_table[i], i};
  }

  stable_sort(srt.begin(), srt.end());

  for (uint16_t i = 0; i < pow(8, D); i++) {
    zigzag_table[srt[i].second] = i;
  }
}

#endif
