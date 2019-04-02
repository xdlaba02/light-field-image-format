/******************************************************************************\
* SOUBOR: endian_t.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <algorithm>

#ifndef ENDIAN_T_H
#define ENDIAN_T_H

template <typename T>
inline T endianSwap(T data) {
  uint8_t *ptr = reinterpret_cast<uint8_t *>(&data);
  std::reverse(ptr, ptr + sizeof(T));
  return data;
}

template <typename T>
inline T bigEndianSwap(T data) {
  if (htobe16(1) == 1) {
    return data;
  }
  else {
    return endianSwap(data);
  }
}

template<typename T>
inline void writeValueToStream(T data, std::ostream &stream) {
  T dataBE = bigEndianSwap(data);
  stream.write(reinterpret_cast<const char *>(&dataBE), sizeof(dataBE));
}

template<typename T>
inline T readValueFromStream(std::istream &stream) {
  T dataBE {};
  stream.read(reinterpret_cast<char *>(&dataBE), sizeof(dataBE));
  return bigEndianSwap(dataBE);
}

#endif
