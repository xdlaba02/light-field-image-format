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

vector<vector<RunLengthPair>> decodePairs(const vector<uint8_t> &huff_counts_DC, const vector<uint8_t> &huff_counts_AC, const vector<uint8_t> &huff_symbols_DC, const vector<uint8_t> &huff_symbols_AC, const uint64_t count, IBitstream &bitstream) {
  vector<vector<RunLengthPair>> runlength(count);

  RunLengthPair pair {};

  for (uint64_t i = 0; i < count; i++) {
    runlength[i].push_back(decodeOnePair(huff_counts_DC, huff_symbols_DC, bitstream));
    do {
      pair = decodeOnePair(huff_counts_AC, huff_symbols_AC, bitstream);
      runlength[i].push_back(pair);
    } while((pair.zeroes != 0) || (pair.amplitude != 0));
  }

  return runlength;
}

RunLengthPair decodeOnePair(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream) {
  uint8_t symbol = decodeOneHuffmanSymbol(counts, symbols, stream);
  int16_t amplitude = decodeOneAmplitude(symbol & 0x0f, stream);
  return {static_cast<uint8_t>(symbol >> 4), amplitude};
}

uint8_t decodeOneHuffmanSymbol(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream) {
  uint16_t code  = 0;
  uint16_t first = 0;
  uint16_t index = 0;
  uint16_t count = 0;

  for (uint8_t len = 1; len < counts.size(); len++) {
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

int16_t decodeOneAmplitude(uint8_t length, IBitstream &stream) {
  int16_t amplitude = 0;

  if (length != 0) {
    for (uint8_t i = 0; i < length; i++) {
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

vector<uint8_t> YCbCrToRGB(const vector<float> &Y_data, const vector<float> &Cb_data, const vector<float> &Cr_data) {
  vector<uint8_t> rgb_data(Y_data.size() * 3);

  for (uint64_t pixel_index = 0; pixel_index < Y_data.size(); pixel_index++) {
    float Y  = Y_data[pixel_index];
    float Cb = Cb_data[pixel_index] - 128;
    float Cr = Cr_data[pixel_index] - 128;

    rgb_data[3*pixel_index + 0] = clamp(Y +                      (1.402 * Cr), 0.0, 255.0);
    rgb_data[3*pixel_index + 1] = clamp(Y - (0.344136 * Cb) - (0.714136 * Cr), 0.0, 255.0);
    rgb_data[3*pixel_index + 2] = clamp(Y + (1.772    * Cb)                  , 0.0, 255.0);
  }

  return rgb_data;
}
