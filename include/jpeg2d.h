#ifndef JPEG2D_H
#define JPEG2D_H

#include <cstdint>
#include <array>
#include <cmath>
#include <vector>
#include <iostream>

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

void printDimensions(uint64_t width, uint64_t height);
void printPairs(std::vector<std::vector<RunLengthPair>> &pairs);

template<typename T>
void printBlock(Block<T> &block) {
  for (uint8_t y = 0; y < 8; y++) {
    for (uint8_t x = 0; x < 8; x++) {
      std::cerr << long(block[y * 8 + x]) << " ";
    }
    std::cerr << std::endl;
  }
}

template<typename T>
void printBuffer(std::vector<T> &buff) {
  for (auto &&v: buff) {
    std::cerr << long(v) << " ";
  }
  std::cerr << std::endl;
}

template<typename T>
void printBlockBuffer(std::vector<Block<T>> &buff) {
  for (auto &v: buff) {
    printBlock(v);
  }
  std::cerr << std::endl;
}
#endif
