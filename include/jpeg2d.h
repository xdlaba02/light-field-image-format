#ifndef JPEG2D_H
#define JPEG2D_H

#include <cstdint>
#include <array>
#include <cmath>

template<typename T>
using Block = std::array<T, 64>;

using QuantTable = Block<uint8_t>;

const double PI = 4 * atan(1);

struct RunLengthPair {
  uint8_t zeroes;
  int16_t amplitude;

  RunLengthPair(uint8_t z = 0, int16_t a = 0): zeroes(z), amplitude(a) {}
};

bool amIBigEndian();
uint64_t swapBytes(uint64_t v);
uint64_t toBigEndian(uint64_t v);
uint64_t fromBigEndian(uint64_t v);

#endif
