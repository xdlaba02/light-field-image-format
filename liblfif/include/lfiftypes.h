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

using RGBUNIT   = uint16_t;
using QDATAUNIT = int64_t;
using YCBCRUNIT = double;

template<typename T>
struct RGBPixel {
  T r;
  T g;
  T b;
};

template<typename T, size_t D>
using Block = std::array<T, static_cast<size_t>(constpow(8, D))>;

template<typename T>
inline void writeValueToStream(std::ofstream &stream, T data) {
  stream.write(reinterpret_cast<const char *>(&data), sizeof(data));
}

template<>
inline void writeValueToStream<uint16_t>(std::ofstream &stream, uint16_t data) {
  data = htobe16(data);
  stream.write(reinterpret_cast<const char *>(&data), sizeof(data));
}

template<>
inline void writeValueToStream<uint32_t>(std::ofstream &stream, uint32_t data) {
  data = htobe32(data);
  stream.write(reinterpret_cast<const char *>(&data), sizeof(data));
}

template<>
inline void writeValueToStream<uint64_t>(std::ofstream &stream, uint64_t data) {
  data = htobe64(data);
  stream.write(reinterpret_cast<const char *>(&data), sizeof(data));
}


template<typename T>
inline T readValueFromStream(std::ifstream &stream) {
  T data {};
  stream.read(reinterpret_cast<char *>(&data), sizeof(data));
  return data;
}

template<>
inline uint16_t readValueFromStream<uint16_t>(std::ifstream &stream) {
  uint16_t data {};
  stream.read(reinterpret_cast<char *>(&data), sizeof(data));
  return be16toh(data);
}

template<>
inline uint32_t readValueFromStream<uint32_t>(std::ifstream &stream) {
  uint32_t data {};
  stream.read(reinterpret_cast<char *>(&data), sizeof(data));
  return be32toh(data);
}

template<>
inline uint64_t readValueFromStream<uint64_t>(std::ifstream &stream) {
  uint64_t data {};
  stream.read(reinterpret_cast<char *>(&data), sizeof(data));
  return be64toh(data);
}

#endif
