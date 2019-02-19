/******************************************************************************\
* SOUBOR: lfiftypes.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIFTYPES_H
#define LFIFTYPES_H

#include "constpow.h"

#include <cstdint>

#include <array>
#include <fstream>

using YCbCrUnit = double;

template<typename T>
struct RGBPixel {
  T r;
  T g;
  T b;
};

template<typename T, size_t D>
using Block = std::array<T, static_cast<size_t>(constpow(8, D))>;

template<typename T>
void writeToStreamHelper(std::ofstream &stream, const T *data, size_t size) {
  stream.write(reinterpret_cast<const char *>(&data), size);
}

template<>
void writeToStreamHelper<uint16_t>(std::ofstream &stream, const uint16_t *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    uint16_t tmp = htobe16(data[i]);
    stream.write(reinterpret_cast<const char *>(&tmp), sizeof(uint16_t));
  }
}

template<typename T>
void readFromStreamHelper(std::ifstream &stream, T *data, size_t size) {
  stream.read(reinterpret_cast<char *>(data), size);
}

template<>
void readFromStreamHelper<uint16_t>(std::ifstream &stream, uint16_t *data, size_t size) {
  stream.read(reinterpret_cast<char *>(data), size * sizeof(uint16_t));

  for (size_t i  = 0; i < size; i++) {
    data[i] = be16toh(data[i]);
  }
}

#endif
