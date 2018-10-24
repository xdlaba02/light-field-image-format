/*******************************************************\
* SOUBOR: jpeg_decoder.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "jpeg_decoder.h"

#include <numeric>

void readHuffmanTable(vector<uint8_t> &counts, vector<uint8_t> &symbols, ifstream &stream) {
  counts.resize(stream.get());
  stream.read(reinterpret_cast<char *>(counts.data()), counts.size());

  symbols.resize(accumulate(counts.begin(), counts.end(), 0, std::plus<uint8_t>()));
  stream.read(reinterpret_cast<char *>(symbols.data()), symbols.size());
}

RunLengthPair decodeOnePair(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream) {
  uint8_t symbol = decodeOneHuffmanSymbol(counts, symbols, stream);
  int16_t amplitude = decodeOneAmplitude(symbol & 0x0f, stream);
  return {static_cast<uint8_t>(symbol >> 4), amplitude};
}

uint8_t decodeOneHuffmanSymbol(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream) {
  uint16_t code  = 0;
  uint16_t first = 0;
  uint16_t index = 0;
  uint16_t count = 0;

  for (uint8_t len = 1; len < counts.size(); len++) {
    code |= stream.readBit();
    count = counts[len];
    if (code - count < first) {
      return symbols[index + (code - first)];
    }
    index += count;
    first += count;
    first <<= 1;
    code <<= 1;
  }

  return symbols.at(0);
}

int16_t decodeOneAmplitude(uint8_t length, IBitstream &stream) {
  int16_t amplitude = 0;

  if (length != 0) {
    for (uint8_t i = 0; i < length; i++) {
      amplitude <<= 1;
      amplitude |= stream.readBit();
    }

    if (amplitude < (1 << (length - 1))) {
      amplitude |= 0xffff << length;
      amplitude = ~amplitude;
      amplitude = -amplitude;
    }
  }

  return amplitude;
}
