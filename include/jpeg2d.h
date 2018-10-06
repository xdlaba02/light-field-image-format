#ifndef JPEG2D_H
#define JPEG2D_H

#include <cstdint>

template<typename T>
using Block = T[64];

using QuantTable = Block<uint8_t>;

struct RunLengthPair {
  uint8_t zeroes;
  int16_t amplitude;

  RunLengthPair(uint8_t z, int16_t a): zeroes(z), amplitude(a) {}
};

bool amIBigEndian();
uint64_t swapBytes(uint64_t v);
uint64_t toBigEndian(uint64_t v);
uint64_t fromBigEndian(uint64_t v);

#endif
