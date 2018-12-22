/******************************************************************************\
* SOUBOR: lfif_decoder.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "lfif_decoder.h"

#include <numeric>

bool checkMagicNumber(const string &cmp, ifstream &input) {
  char magic_number[9] {};
  input.read(magic_number, 8);

  if (string(magic_number) != cmp) {
    return false;
  }
  else {
    return true;
  }
}

uint64_t readDimension(ifstream &input) {
  uint64_t raw {};
  input.read(reinterpret_cast<char *>(&raw), sizeof(uint64_t));
  return be64toh(raw);
}

HuffmanTable readHuffmanTable(ifstream &stream) {
  HuffmanTable table {};

  table.counts.resize(stream.get());
  stream.read(reinterpret_cast<char *>(table.counts.data()), table.counts.size());

  table.symbols.resize(accumulate(table.counts.begin(), table.counts.end(), 0, plus<uint8_t>()));
  stream.read(reinterpret_cast<char *>(table.symbols.data()), table.symbols.size());

  return table;
}

void decodeOneBlock(RunLengthEncodedBlock &pairs, const HuffmanTable &hufftable_DC, const HuffmanTable &hufftable_AC, IBitstream &bitstream) {
  RunLengthPair pair = decodeOnePair(hufftable_DC, bitstream);
  pairs.push_back(pair);
  do {
    pair = decodeOnePair(hufftable_AC, bitstream);
    pairs.push_back(pair);
  } while((pair.zeroes != 0) || (pair.amplitude != 0));
}

RunLengthPair decodeOnePair(const HuffmanTable &table, IBitstream &stream) {
  size_t symbol_index = decodeOneHuffmanSymbolIndex(table.counts, stream);
  RunLengthAmplitudeUnit amplitude = decodeOneAmplitude(table.symbols[symbol_index] & 0x0f, stream);
  return {static_cast<RunLengthZeroesCountUnit>(table.symbols[symbol_index] >> 4), amplitude};
}

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

RunLengthAmplitudeUnit decodeOneAmplitude(HuffmanSymbol length, IBitstream &stream) {
  RunLengthAmplitudeUnit amplitude = 0;

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

RunLengthEncodedImage diffDecodePairs(RunLengthEncodedImage runlengths) {
  RunLengthAmplitudeUnit prev_DC = 0;

  for (auto &runlength_block: runlengths) {
    runlength_block[0].amplitude += prev_DC;
    prev_DC = runlength_block[0].amplitude;
  }

  return runlengths;
}

YCbCrData deshiftData(YCbCrData data) {
  for (auto &pixel: data) {
    pixel += 128;
  }
  return data;
}

RGBData YCbCrToRGB(const YCbCrData &Y_data, const YCbCrData &Cb_data, const YCbCrData &Cr_data) {
  RGBData rgb_data(Y_data.size() * 3);

  for (size_t pixel_index = 0; pixel_index < Y_data.size(); pixel_index++) {
    YCbCrDataUnit Y  = Y_data[pixel_index];
    YCbCrDataUnit Cb = Cb_data[pixel_index] - 128;
    YCbCrDataUnit Cr = Cr_data[pixel_index] - 128;

    rgb_data[3*pixel_index + 0] = clamp(Y +                      (1.402 * Cr), 0.0, 255.0);
    rgb_data[3*pixel_index + 1] = clamp(Y - (0.344136 * Cb) - (0.714136 * Cr), 0.0, 255.0);
    rgb_data[3*pixel_index + 2] = clamp(Y + (1.772    * Cb)                  , 0.0, 255.0);
  }

  return rgb_data;
}
