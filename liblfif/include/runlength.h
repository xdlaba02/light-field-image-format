/******************************************************************************\
* SOUBOR: runlength.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef RUNLENGTH_H
#define RUNLENGTH_H

#include <cstdint>

#include "huffman.h"

using RLAMPUNIT = int64_t;
using HuffmanClass = uint8_t;

class RunLengthPair {
public:
  size_t    zeroes;
  RLAMPUNIT amplitude;

  RunLengthPair &addToWeights(HuffmanWeights &weights, size_t amp_bits);

  RunLengthPair &huffmanEncodeToStream(HuffmanEncoder &encoder, OBitstream &stream, size_t amp_bits);
  RunLengthPair &huffmanDecodeFromStream(HuffmanDecoder &decoder, IBitstream &stream, size_t amp_bits);

  bool endOfBlock() const;

  static size_t zeroesBits(size_t amp_bits);
  static size_t classBits(size_t amp_bits);

private:
  HuffmanClass  huffmanClass() const;
  HuffmanSymbol huffmanSymbol(size_t amp_bits) const;
};

#endif
