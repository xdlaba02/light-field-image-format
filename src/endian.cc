/*******************************************************\
* SOUBOR: endian.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "endian.h"

bool amIBigEndian() {
  uint16_t test = 0;
  return reinterpret_cast<char *>(&test)[0] == 0;
}

uint64_t swapBytes(uint64_t v) {
  return ((v & 0xFF00000000000000u) >> 56u) |
         ((v & 0x00FF000000000000u) >> 40u) |
         ((v & 0x0000FF0000000000u) >> 24u) |
         ((v & 0x000000FF00000000u) >>  8u) |
         ((v & 0x00000000FF000000u) <<  8u) |
         ((v & 0x0000000000FF0000u) << 24u) |
         ((v & 0x000000000000FF00u) << 40u) |
         ((v & 0x00000000000000FFu) << 56u);
}

uint64_t toBigEndian(uint64_t v) {
  if (amIBigEndian()) {
    return v;
  }
  else {
    return swapBytes(v);
  }
}

uint64_t fromBigEndian(uint64_t v) {
  if (amIBigEndian()) {
    return v;
  }
  else {
    return swapBytes(v);
  }
}
