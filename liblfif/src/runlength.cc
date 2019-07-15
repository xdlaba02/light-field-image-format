#include "runlength.h"
#include "bitstream.h"

void RunLengthPair::huffmanEncodeToStream(const HuffmanEncoder &encoder, OBitstream &stream, size_t class_bits) const {
  HuffmanClass amp_class {};
  RLAMPUNIT    amp       {};

  encoder.encodeSymbolToStream(huffmanSymbol(class_bits), stream);

  amp = amplitude;
  if (amp < 0) {
    amp = -amp;
    amp = ~amp;
  }

  amp_class = huffmanClass();
  for (int16_t i = amp_class - 1; i >= 0; i--) {
    stream.writeBit((amp & (1 << i)) >> i);
  }
}

void RunLengthPair::huffmanDecodeFromStream(const HuffmanDecoder &decoder, IBitstream &stream, size_t class_bits) {
  HuffmanClass  amp_class      {};
  HuffmanSymbol huffman_symbol {};

  huffman_symbol  = decoder.decodeSymbolFromStream(stream);
  amp_class       = huffman_symbol & (~(static_cast<HuffmanSymbol>(-1) << class_bits));
  zeroes          = huffman_symbol >> class_bits;
  amplitude       = 0;

  if (amp_class != 0) {
    for (HuffmanClass i = 0; i < amp_class; i++) {
      amplitude <<= 1;
      amplitude |= stream.readBit();
    }

    if (amplitude < (1 << (amp_class - 1))) {
      amplitude |= static_cast<uint64_t>(-1) << amp_class;
      amplitude = ~amplitude;
      amplitude = -amplitude;
    }
  }
}

void RunLengthPair::CABACEncodeToStream(CABACEncoder &encoder, CABAC::ContextModel models[8+8+14], size_t class_bits) const {
  HuffmanClass amp_class {};
  RLAMPUNIT    amp       {};

  HuffmanSymbol symbol = huffmanSymbol(class_bits);

  for (size_t i = 0; i < sizeof(HuffmanSymbol) * 8; i++) {
    encoder.encodeBit(models[i], (symbol >> i) & 1);
  }

  amp = amplitude;
  if (amp < 0) {
    amp = -amp;
    amp = ~amp;
  }

  amp_class = huffmanClass();
  for (size_t i = 0; i < amp_class; i++) {
    encoder.encodeBit(models[i < 13 ? 16 + i : 16 + 13], (amp >> i) & 1);
  }
}

void RunLengthPair::CABACDecodeFromStream(CABACDecoder &decoder, CABAC::ContextModel models[8+8+14], size_t class_bits) {
  HuffmanClass  amp_class      {};
  HuffmanSymbol huffman_symbol {};

  for (size_t i = 0; i < sizeof(HuffmanSymbol) * 8; i++) {
    huffman_symbol |= decoder.decodeBit(models[i]) << i;
  }

  amp_class       = huffman_symbol & (~(static_cast<HuffmanSymbol>(-1) << class_bits));
  zeroes          = huffman_symbol >> class_bits;
  amplitude       = 0;

  if (amp_class != 0) {
    for (size_t i = 0; i < amp_class; i++) {
      amplitude |= decoder.decodeBit(models[i < 13 ? 16 + i : 16 + 13]) << i;
    }

    if (amplitude < (1 << (amp_class - 1))) {
      amplitude |= static_cast<uint64_t>(-1) << amp_class;
      amplitude = ~amplitude;
      amplitude = -amplitude;
    }
  }
}
