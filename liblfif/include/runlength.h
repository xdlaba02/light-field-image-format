/******************************************************************************\
* SOUBOR: runlength.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef RUNLENGTH_H
#define RUNLENGTH_H

#include <cstdint>

#include "huffman.h"

using HuffmanClass = uint8_t;

template <typename T>
class RunLengthPair {
public:
  size_t zeroes;
  T      amplitude;

  RunLengthPair &addToWeights(HuffmanWeights &weights);

  RunLengthPair &huffmanEncodeToStream(HuffmanEncoder &encoder, OBitstream &stream);
  RunLengthPair &huffmanDecodeFromStream(HuffmanDecoder &decoder, IBitstream &stream);

  bool endOfBlock() const;

  static size_t zeroesBits();
  static size_t classBits();

private:
  HuffmanClass  huffmanClass() const;
  HuffmanSymbol huffmanSymbol() const;
};

#endif
