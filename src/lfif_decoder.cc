/*******************************************************\
* SOUBOR: lfif_decoder.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "lfif_decoder.h"

#include <numeric>

void readHuffmanTable(vector<uint8_t> &counts, vector<uint8_t> &symbols, ifstream &stream) {
  counts.resize(stream.get());
  stream.read(reinterpret_cast<char *>(counts.data()), counts.size());

  symbols.resize(accumulate(counts.begin(), counts.end(), 0, std::plus<uint8_t>()));
  stream.read(reinterpret_cast<char *>(symbols.data()), symbols.size());
}

RunLengthPair decodeOnePair(const vector<uint8_t> &counts, const vector<HuffmanSymbol> &symbols, IBitstream &stream) {
  HuffmanSymbol symbol = decodeOneHuffmanSymbol(counts, symbols, stream);
  RunLengthAmplitudeUnit amplitude = decodeOneAmplitude(symbol & 0x0f, stream);
  return {static_cast<RunLengthZeroesCountUnit>(symbol >> 4), amplitude};
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

HuffmanSymbol decodeOneHuffmanSymbol(const vector<uint8_t> &counts, const vector<HuffmanSymbol> &symbols, IBitstream &stream) {
  uint16_t code  = 0;
  uint16_t first = 0;
  uint16_t index = 0;
  uint16_t count = 0;

  for (size_t len = 1; len < counts.size(); len++) {
    code |= stream.readBit();
    count = counts[len];
    if (code - count < first) {
      return symbols[index + (code - first)];
    }
    index += count;
    first += count;
    first <<= 1;
    code <<= 1;
  }

  return symbols.at(0);
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
