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

const double JPEG_PI = 4 * atan(1);

struct RunLengthPair {
  uint8_t zeroes;
  int16_t amplitude;
};

template<uint8_t D>
uint16_t zigzagIndexTable(uint16_t index) {
  const Block<uint8_t, 2> zigzag_index_table_2D {
     0, 1, 5, 6,14,15,27,28,
     2, 4, 7,13,16,26,29,42,
     3, 8,12,17,25,30,41,43,
     9,11,18,24,31,40,44,53,
    10,19,23,32,39,45,52,54,
    20,22,33,38,46,51,55,60,
    21,34,37,47,50,56,59,61,
    35,36,48,49,57,58,62,63
  };

  if (D == 2) {
    return zigzag_index_table_2D[index];
  }
  else {
    return index;
  }
}

#endif
