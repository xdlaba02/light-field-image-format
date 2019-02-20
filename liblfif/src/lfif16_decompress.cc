/******************************************************************************\
* SOUBOR: lfif16_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "lfif16.h"
#include "bitstream.h"

#include <cstddef>
#include <cmath>

#include <array>
#include <map>
#include <bitset>
#include <algorithm>
#include <fstream>
#include <numeric>

using namespace std;

inline void putBlock(uint16_t *rgb_data, const uint64_t img_dims[2], size_t block, const uint16_t in_block[64*3]) {
  size_t blocks_x = ceil(img_dims[0]/8.0);
  size_t size_x   = img_dims[0];

  for (size_t pixel_y = 0; pixel_y < 8; pixel_y++) {
    size_t image_y = (block / blocks_x) * 8 + pixel_y;

    if (image_y >= img_dims[1]) {
      break;
    }

    for (size_t pixel_x = 0; pixel_x < 8; pixel_x++) {
      size_t image_x = block % blocks_x * 8 + pixel_x;

      if (image_x >= img_dims[0]) {
        break;
      }

      rgb_data[(image_y * size_x + image_x) * 3 + 0] = in_block[(pixel_y * 8 + pixel_x) * 3 + 0];
      rgb_data[(image_y * size_x + image_x) * 3 + 1] = in_block[(pixel_y * 8 + pixel_x) * 3 + 1];
      rgb_data[(image_y * size_x + image_x) * 3 + 2] = in_block[(pixel_y * 8 + pixel_x) * 3 + 2];
    }
  }
}

inline void YCbCrDataBlockToRGBDataBlock(const double input_Y[64], const double input_Cb[64], const double input_Cr[64], uint16_t output[3*64]) {
  for (size_t pixel_index = 0; pixel_index < 64; pixel_index++) {
    double Y  = input_Y[pixel_index];
    double Cb = input_Cb[pixel_index] - 32768;
    double Cr = input_Cr[pixel_index] - 32768;

    output[pixel_index * 3 + 0] = clamp(Y +                      (1.402 * Cr), 0.0, 65535.0);
    output[pixel_index * 3 + 1] = clamp(Y - (0.344136 * Cb) - (0.714136 * Cr), 0.0, 65535.0);
    output[pixel_index * 3 + 2] = clamp(Y + (1.772    * Cb)                  , 0.0, 65535.0);
  }
}

inline void deshiftBlock(double input[64]) {
  for (size_t i = 0; i < 64; i++) {
    input[i] += 32768;
  }
}

inline void dequantizeBlock(const int32_t input[64], const uint16_t quant_table[64], double output[64]) {
  for (size_t pixel_index = 0; pixel_index < 64; pixel_index++) {
    output[pixel_index] = input[pixel_index] * quant_table[pixel_index];
  }
}

inline void detraverseBlock(int32_t input[64], const uint8_t traversal_table[64]) {
  int32_t tmp[64] {};
  for (size_t i = 0; i < 64; i++) {
    tmp[i] = input[traversal_table[i]];
  }
  for (size_t i = 0; i < 64; i++) {
    input[i] = tmp[i];
  }
}

inline void diffDecodeDC(int32_t &input, int32_t &prev) {
  input += prev;
  prev = input;
}

inline void runLenghtDecodeBlock(const vector<RunLengthPair> &input, int32_t output[64]) {
  size_t pixel_index = 0;
  for (auto &pair: input) {
    pixel_index += pair.zeroes;
    output[pixel_index] = pair.amplitude;
    pixel_index++;
  }
}

inline uint64_t readDimension(ifstream &input) {
  uint64_t raw {};
  input.read(reinterpret_cast<char *>(&raw), sizeof(raw));
  return be64toh(raw);
}

inline void readQuantTable(ifstream &input, uint16_t quant_table[64]) {
  for (size_t i = 0; i < 64; i++) {
    input.read(reinterpret_cast<char *>(&quant_table[i]), sizeof(uint16_t));
    quant_table[i] = be16toh(quant_table[i]);
  }
}

inline void readTraversalTable(ifstream &input, uint8_t traversal_table[64]) {
  input.read(reinterpret_cast<char *>(traversal_table), 64);
}

inline HuffmanTable readHuffmanTable(ifstream &input) {
  HuffmanTable table {};

  table.counts.resize(input.get());
  input.read(reinterpret_cast<char *>(table.counts.data()), table.counts.size());

  table.symbols.resize(accumulate(table.counts.begin(), table.counts.end(), 0, plus<uint8_t>()));
  input.read(reinterpret_cast<char *>(table.symbols.data()), table.symbols.size());

  return table;
}

inline size_t decodeOneHuffmanSymbolIndex(const vector<uint8_t> &counts, IBitstream &stream) {
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

int32_t decodeOneAmplitude(uint8_t length, IBitstream &stream) {
  int32_t amplitude = 0;

  if (length != 0) {
    for (uint8_t i = 0; i < length; i++) {
      amplitude <<= 1;
      amplitude |= stream.readBit();
    }

    if (amplitude < (1 << (length - 1))) {
      amplitude |= 0xffffffff << length;
      amplitude = ~amplitude;
      amplitude = -amplitude;
    }
  }

  return amplitude;
}

RunLengthPair decodeOnePair(const HuffmanTable &table, IBitstream &stream) {
  uint8_t symbol = table.symbols[decodeOneHuffmanSymbolIndex(table.counts, stream)];
  int32_t amplitude = decodeOneAmplitude(symbol & 0x1f, stream);
  return {static_cast<size_t>(symbol >> 5), amplitude};
}

vector<RunLengthPair> decodeOneBlock(const HuffmanTable hufftables[2], IBitstream &bitstream) {
  vector<RunLengthPair> output {};

  RunLengthPair pair {};

  output.push_back(decodeOnePair(hufftables[0], bitstream));
  do {
    pair = decodeOnePair(hufftables[1], bitstream);
    output.push_back(pair);
  } while((pair.zeroes != 0) || (pair.amplitude != 0));

  return output;
}

inline bool checkMagicNumber(const string &cmp, ifstream &input) {
  char magic_number[9] {};
  input.read(magic_number, 8);
  return string(magic_number) == cmp;
}

int LFIFDecompress16(const char *input_file_name, vector<uint16_t> &rgb_data, uint64_t img_dims[2]) {
  ifstream input {};

  uint16_t quant_table_luma[64]   {};
  uint16_t quant_table_chroma[64] {};

  uint8_t traversal_table_luma[64]   {};
  uint8_t traversal_table_chroma[64] {};

  HuffmanTable hufftable_luma[2]   {};
  HuffmanTable hufftable_chroma[2] {};

  size_t blocks_cnt {};
  size_t pixels_cnt {};

  int32_t previous_DCs[3] {};

  HuffmanTable *hufftables[3]  {};
  uint8_t *traversal_tables[3] {};
  uint16_t *quant_tables[3]    {};

  hufftables[0] = hufftable_luma;
  hufftables[1] = hufftable_chroma;
  hufftables[2] = hufftable_chroma;

  traversal_tables[0] = traversal_table_luma;
  traversal_tables[1] = traversal_table_chroma;
  traversal_tables[2] = traversal_table_chroma;

  quant_tables[0] = quant_table_luma;
  quant_tables[1] = quant_table_chroma;
  quant_tables[2] = quant_table_chroma;

  input.open(input_file_name);
  if (input.fail()) {
    return -1;
  }

  if (!checkMagicNumber("LFIF-16\n", input)) {
    return -2;
  }

  img_dims[0] = readDimension(input);
  img_dims[1] = readDimension(input);

  readQuantTable(input, quant_table_luma);
  readQuantTable(input, quant_table_chroma);

  readTraversalTable(input, traversal_table_luma);
  readTraversalTable(input, traversal_table_chroma);

  hufftable_luma[0] = readHuffmanTable(input);
  hufftable_luma[1] = readHuffmanTable(input);
  hufftable_chroma[0] = readHuffmanTable(input);
  hufftable_chroma[1] = readHuffmanTable(input);

  blocks_cnt = ceil(img_dims[0]/8.) * ceil(img_dims[1]/8.);
  pixels_cnt = img_dims[0] * img_dims[1];

  rgb_data.resize(pixels_cnt * 3);

  IBitstream bitstream(input);

  previous_DCs[0] = 0;
  previous_DCs[1] = 0;
  previous_DCs[2] = 0;

  for (size_t block = 0; block < blocks_cnt; block++) {
    uint16_t block_rgb[64*3]  {};
    double  blocks_raw[3][64] {};

    for (size_t channel = 0; channel < 3; channel++) {
      int32_t block_quant[64]   {};
      double  blocks_coefs[64]  {};

      runLenghtDecodeBlock(decodeOneBlock(hufftables[channel], bitstream), block_quant);
      diffDecodeDC(block_quant[0], previous_DCs[channel]);
      detraverseBlock(block_quant, traversal_tables[channel]);
      dequantizeBlock(block_quant, quant_tables[channel], blocks_coefs);
      idct<2>([&](size_t index){ return blocks_coefs[index];}, [&](size_t index) -> double & { return blocks_raw[channel][index];});
      deshiftBlock(blocks_raw[channel]);
    }

    YCbCrDataBlockToRGBDataBlock(blocks_raw[0], blocks_raw[1], blocks_raw[2], block_rgb);
    putBlock(rgb_data.data(), img_dims, block, block_rgb);
  }

  return 0;
}
