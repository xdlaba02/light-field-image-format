#include "runlength.h"
#include "bitstream.h"

RunLengthPair &RunLengthPair::addToWeights(HuffmanWeights &weights) {
  HuffmanClass  amp_class {};
  HuffmanSymbol symbol    {};

  amp_class = huffmanClass() & 0x0f;
  symbol    = zeroes << 4 | amp_class;

  weights[symbol]++;

  return *this;
}

RunLengthPair &RunLengthPair::huffmanEncodeToStream(HuffmanEncoder &encoder, OBitstream &stream) {
  HuffmanSymbol          symbol    {};
  HuffmanClass           amp_class {};
  RunLengthAmplitudeUnit amp       {};

  amp_class = huffmanClass() & 0x0f;
  symbol    = zeroes << 4 | amp_class;
  encoder.encodeSymbolToStream(symbol, stream);

  amp = amplitude;
  if (amp < 0) {
    amp = -amp;
    amp = ~amp;
  }

  for (int16_t i = amp_class - 1; i >= 0; i--) {
    stream.writeBit((amp & (1 << i)) >> i);
  }

  return *this;
}

RunLengthPair &RunLengthPair::huffmanDecodeFromStream(HuffmanDecoder &decoder, IBitstream &stream) {
  HuffmanSymbol huffman_symbol {};
  HuffmanClass  amp_class      {};

  huffman_symbol  = decoder.decodeSymbolFromStream(stream);
  amp_class       = huffman_symbol & 0x0f;
  zeroes          = huffman_symbol >> 4;
  amplitude       = 0;

  if (amp_class != 0) {
    for (HuffmanClass i = 0; i < amp_class; i++) {
      amplitude <<= 1;
      amplitude |= stream.readBit();
    }

    if (amplitude < (1 << (amp_class - 1))) {
      amplitude |= static_cast<unsigned>(-1) << amp_class;
      amplitude = ~amplitude;
      amplitude = -amplitude;
    }
  }

  return *this;
}

bool RunLengthPair::endOfBlock() const {
  return (!zeroes) && (!amplitude);
}

HuffmanClass RunLengthPair::huffmanClass() const {
  RunLengthAmplitudeUnit amp = amplitude;
  if (amp < 0) {
    amp = -amp;
  }

  HuffmanClass huff_class = 0;
  while (amp > 0) {
    amp >>= 1;
    huff_class++;
  }

  return huff_class;
}
