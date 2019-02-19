#include "runlength.h"
#include "bitstream.h"

#include <cmath>

template class RunLengthPair<int16_t>;
template class RunLengthPair<int32_t>;

template <typename T>
RunLengthPair<T> &
RunLengthPair<T>::addToWeights(HuffmanWeights &weights) {
  weights[HuffmanSymbol()]++;

  return *this;
}

template <typename T>
RunLengthPair<T> &
RunLengthPair<T>::huffmanEncodeToStream(HuffmanEncoder &encoder, OBitstream &stream) {
  HuffmanClass amp_class {};
  T            amp       {};

  encoder.encodeSymbolToStream(huffmanSymbol(), stream);

  amp = amplitude;
  if (amp < 0) {
    amp = -amp;
    amp = ~amp;
  }

  amp_class = huffmanClass();
  for (int16_t i = amp_class - 1; i >= 0; i--) {
    stream.writeBit((amp & (1 << i)) >> i);
  }

  return *this;
}

template <typename T>
RunLengthPair<T> &
RunLengthPair<T>::huffmanDecodeFromStream(HuffmanDecoder &decoder, IBitstream &stream) {
  HuffmanSymbol huffman_symbol {};
  HuffmanClass  amp_class      {};

  huffman_symbol  = decoder.decodeSymbolFromStream(stream);
  amp_class       = huffman_symbol & (~(static_cast<HuffmanSymbol>(-1) << classBits()));
  zeroes          = huffman_symbol >> classBits();
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

template <typename T>
bool RunLengthPair<T>::endOfBlock() const {
  return (!zeroes) && (!amplitude);
}

template <typename T>
size_t RunLengthPair<T>::zeroesBits() {
  return 8 - classBits();
}

template <typename T>
size_t RunLengthPair<T>::classBits() {
  return log2(sizeof(T)*8);
}

template <typename T>
HuffmanClass RunLengthPair<T>::huffmanClass() const {
  T amp = amplitude;
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

template <typename T>
HuffmanSymbol RunLengthPair<T>::huffmanSymbol() const {
  return zeroes << classBits() | huffmanClass();
}
