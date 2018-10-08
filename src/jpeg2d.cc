#include "jpeg2d.h"

bool amIBigEndian() {
  uint16_t test = 1;
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

void printDimensions(uint64_t width, uint64_t height) {
  std::cerr << long(width) << " " << long(height) << std::endl;
}

void printPairs(std::vector<std::vector<RunLengthPair>> &pairs) {
  for (auto &v: pairs) {
    for (auto &p: v) {
      std::cerr << "(" << long(p.zeroes) << "," << long(p.amplitude) << ") ";
    }
    std::cerr << std::endl;
  }
  std::cerr << std::endl;
}
