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

#endif
