/******************************************************************************\
* SOUBOR: lfiftypes.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIFTYPES_H
#define LFIFTYPES_H

#include "constpow.h"
#include "huffman.h"
#include "bitstream.h"

#include <cstdint>

#include <vector>


using namespace std;

using RGBDataUnit = uint8_t;
using YCbCrUnit = double;

using RunLengthZeroesCountUnit = uint8_t;

using HuffmanClass = uint8_t;

using QuantizedDataUnit = int16_t;
using ReferenceBlockUnit = double;
using QuantTableUnit = uint8_t;
using TraversalTableUnit = uint16_t;

struct RGBPixel {
  RGBDataUnit r;
  RGBDataUnit g;
  RGBDataUnit b;
};

struct RunLengthAmplitudeUnit {
  QuantizedDataUnit value;

  HuffmanClass huffmanClass() const {
    QuantizedDataUnit ampval = value;
    if (ampval < 0) {
      ampval = -ampval;
    }

    HuffmanClass huff_class = 0;
    while (ampval > 0) {
      ampval >>= 1;
      huff_class++;
    }

    return huff_class;
  }
};

struct RunLengthPair {
  uint8_t zeroes;
  RunLengthAmplitudeUnit amplitude;

  HuffmanSymbol huffmanSymbol() const {
    return zeroes << 4 | amplitude.huffmanClass();
  }

  RunLengthPair &huffmanEncodeToStream(const HuffmanMap &map, OBitstream &stream) {
    HuffmanSymbol symbol = huffmanSymbol();
    stream.write(map.at(symbol));

    QuantizedDataUnit ampval = amplitude.value;
    if (ampval < 0) {
      ampval = -ampval;
      ampval = ~ampval;
    }

    for (int8_t i = amplitude.huffmanClass() - 1; i >= 0; i--) {
      stream.writeBit((ampval & (1 << i)) >> i);
    }

    return *this;
  }

  //TODO Encoding / decoding huffman tables

  RunLengthPair &huffmanDecodeFromStream(const HuffmanTable &table, IBitstream &stream) {
    size_t symbol_index = decodeOneHuffmanSymbolIndex(table.counts, stream);
    QuantizedDataUnit amplitude = decodeOneAmplitude(table.symbols[symbol_index] & 0x0f, stream);
    return {static_cast<RunLengthZeroesCountUnit>(table.symbols[symbol_index] >> 4), amplitude};

    return *this
  }
};

template<typename T, size_t D>
using Block = array<T, static_cast<size_t>(constpow(8, D))>;

template<size_t D>
using ReferenceBlock = Block<ReferenceBlockUnit, D>;

#endif
