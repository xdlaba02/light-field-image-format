/******************************************************************************\
* SOUBOR: runlength.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef RUNLENGTH_H
#define RUNLENGTH_H

#include <cstdint>

#include "huffman.h"

using RunLengthZeroesCountUnit = uint8_t;
using RunLengthAmplitudeUnit   = int16_t;
using HuffmanClass             = uint8_t;

struct RunLengthPair {
  RunLengthZeroesCountUnit zeroes;
  RunLengthAmplitudeUnit   amplitude;

  RunLengthPair &addToWeights(HuffmanWeights &weights);

  RunLengthPair &huffmanEncodeToStream(HuffmanEncoder &encoder, OBitstream &stream);
  RunLengthPair &huffmanDecodeFromStream(HuffmanDecoder &decoder, IBitstream &stream);

  bool endOfBlock() const;

private:
  HuffmanClass huffmanClass() const;
};

#endif
