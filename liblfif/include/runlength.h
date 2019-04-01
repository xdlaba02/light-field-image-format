/******************************************************************************\
* SOUBOR: runlength.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef RUNLENGTH_H
#define RUNLENGTH_H

#include <cstdint>
#include <cmath>

#include "huffman.h"

using RLAMPUNIT = int64_t;
using HuffmanClass = uint8_t;

class RunLengthPair {
public:
  size_t    zeroes;
  RLAMPUNIT amplitude;

  RunLengthPair &addToWeights(HuffmanWeights &weights, size_t class_bits) {
    weights[huffmanSymbol(class_bits)]++;
    return *this;
  }

  RunLengthPair &huffmanEncodeToStream(HuffmanEncoder &encoder, OBitstream &stream, size_t class_bits);
  RunLengthPair &huffmanDecodeFromStream(HuffmanDecoder &decoder, IBitstream &stream, size_t class_bits);

  bool eob() const {
    return (!zeroes) && (!amplitude);
  }

  static size_t zeroesBits(size_t class_bits) {
    return sizeof(HuffmanSymbol) * 8 - class_bits;
  }

  static size_t classBits(size_t amp_bits) {
    return ceil(log2(amp_bits));
  }

private:
  HuffmanClass  huffmanClass() const {
    RLAMPUNIT amp = amplitude;
    if (amp < 0) {
      amp = -amp;
    }

    HuffmanClass huff_class = 0;
    while (amp) {
      amp >>= 1;
      huff_class++;
    }

    return huff_class;
  }

  HuffmanSymbol huffmanSymbol(size_t class_bits) const {
    return zeroes << class_bits | huffmanClass();
  }
};

#endif
