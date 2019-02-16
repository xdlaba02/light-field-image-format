/******************************************************************************\
* SOUBOR: huffman.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "huffman.h"

#include <numeric>
#include <algorithm>
#include <fstream>

void writeHuffmanTable(const HuffmanCodelengths &codelengths, ofstream &stream) {
  uint8_t codelengths_cnt = codelengths.back().first + 1;
  stream.put(codelengths_cnt);

  auto it = codelengths.begin();
  for (uint8_t i = 0; i < codelengths_cnt; i++) {
    size_t leaves = 0;
    while ((it < codelengths.end()) && ((*it).first == i)) {
      leaves++;
      it++;
    }
    stream.put(leaves);
  }

  for (auto &pair: codelengths) {
    stream.put(pair.second);
  }
}

/*

size_t decodeOneHuffmanSymbolIndex(const vector<uint8_t> &counts, IBitstream &stream) {
  uint16_t code  = 0;
  uint16_t first = 0;
  uint16_t index = 0;
  uint16_t count = 0;

  for (size_t len = 1; len < counts.size(); len++) {
    code |= stream.readBit();
    count = counts[len];
    if (code - count < first) {
      return index + (code - first);
    }
    index += count;
    first += count;
    first <<= 1;
    code <<= 1;
  }

  return 0;
}

QuantizedDataUnit decodeOneAmplitude(HuffmanSymbol length, IBitstream &stream) {
  QuantizedDataUnit amplitude = 0;

  if (length != 0) {
    for (HuffmanSymbol i = 0; i < length; i++) {
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

HuffmanTable readHuffmanTable(ifstream &stream) {
  HuffmanTable table {};

  table.counts.resize(stream.get());
  stream.read(reinterpret_cast<char *>(table.counts.data()), table.counts.size());

  table.symbols.resize(accumulate(table.counts.begin(), table.counts.end(), 0, plus<uint8_t>()));
  stream.read(reinterpret_cast<char *>(table.symbols.data()), table.symbols.size());

  return table;
}
*/
