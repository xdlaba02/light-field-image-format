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
uint8_t universalQuantTable(uint16_t index) {
  const QuantTable<2> universal_quant_table_2D {
    16,11,10,16, 24, 40, 51, 61,
    12,12,14,19, 26, 58, 60, 55,
    14,13,16,24, 40, 57, 69, 56,
    14,17,22,29, 51, 87, 80, 62,
    18,22,37,56, 68,109,103, 77,
    24,35,55,64, 81,104,113, 92,
    49,64,78,87,103,121,120,101,
    72,92,95,98,112,100,103, 99
  };

  if (D == 2) {
    return universal_quant_table_2D[index];
  }
  else if (D == 3) {
    uint8_t x = index % 8;
    uint8_t y = index / 8;
    uint8_t z = index / 64;

    return (universalQuantTable<2>(x*8 + y)
          + universalQuantTable<2>(y*8 + x)
          + universalQuantTable<2>(y*8 + z)
          + universalQuantTable<2>(z*8 + y)
          + universalQuantTable<2>(z*8 + x)
          + universalQuantTable<2>(x*8 + z)) / 6;
  }
  else {
    return universal_quant_table_2D[index%64];
  }


}

template<uint8_t D>
uint8_t zigzagIndexTable(uint16_t index) {
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
