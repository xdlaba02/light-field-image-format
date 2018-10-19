/*******************************************************\
* SOUBOR: jpeg2d_decoder.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "jpeg2d.h"
#include "ppm.h"

#include <cstring>
#include <fstream>
#include <bitset>
#include <iostream>
#include <numeric>
#include <algorithm>

bool JPEG2D2PPM(const char *input_filename, const char *output_filename) {
  ifstream input(input_filename);
  if (input.fail()) {
    return false;
  }

  char magic_number[8] {};

  input.read(magic_number, 8);
  if (strncmp(magic_number, "JPEG-2D\n", 8) != 0) {
    return false;
  }

  uint64_t raw_width  {};
  uint64_t raw_height {};

  input.read(reinterpret_cast<char *>(&raw_width), sizeof(uint64_t));
  input.read(reinterpret_cast<char *>(&raw_height), sizeof(uint64_t));

  uint64_t width  = fromBigEndian(raw_width);
  uint64_t height = fromBigEndian(raw_height);

  QuantTable quant_table_luma   {};
  QuantTable quant_table_chroma {};

  input.read(reinterpret_cast<char *>(quant_table_luma.data()), quant_table_luma.size());
  input.read(reinterpret_cast<char *>(quant_table_chroma.data()), quant_table_luma.size());

  vector<uint8_t> huff_counts_luma_DC   {};
  vector<uint8_t> huff_counts_luma_AC   {};
  vector<uint8_t> huff_counts_chroma_DC {};
  vector<uint8_t> huff_counts_chroma_AC {};

  vector<uint8_t> huff_symbols_luma_DC   {};
  vector<uint8_t> huff_symbols_luma_AC   {};
  vector<uint8_t> huff_symbols_chroma_DC {};
  vector<uint8_t> huff_symbols_chroma_AC {};

  readHuffmanTable(huff_counts_luma_DC, huff_symbols_luma_DC, input);
  readHuffmanTable(huff_counts_luma_AC, huff_symbols_luma_AC, input);
  readHuffmanTable(huff_counts_chroma_DC, huff_symbols_chroma_DC, input);
  readHuffmanTable(huff_counts_chroma_AC, huff_symbols_chroma_AC, input);

  uint64_t blocks_width  = ceil(width/8.0);
  uint64_t blocks_height = ceil(height/8.0);

  int16_t prev_Y_DC  = 0;
  int16_t prev_Cb_DC = 0;
  int16_t prev_Cr_DC = 0;

  vector<uint8_t> rgb_data(width * height * 3);

  IBitstream bitstream(input);

  for (uint64_t block_y = 0; block_y < blocks_height; block_y++) {
    for (uint64_t block_x = 0; block_x < blocks_width; block_x++) {

      Block<int16_t> block_Y  {};
      Block<int16_t> block_Cb {};
      Block<int16_t> block_Cr {};

      readOneBlock(huff_counts_luma_DC, huff_symbols_luma_DC, huff_counts_luma_AC, huff_symbols_luma_AC, quant_table_luma, prev_Y_DC, block_Y, bitstream);
      readOneBlock(huff_counts_chroma_DC, huff_symbols_chroma_DC, huff_counts_chroma_AC, huff_symbols_chroma_AC, quant_table_chroma, prev_Cb_DC, block_Cb, bitstream);
      readOneBlock(huff_counts_chroma_DC, huff_symbols_chroma_DC, huff_counts_chroma_AC, huff_symbols_chroma_AC, quant_table_chroma, prev_Cr_DC, block_Cr, bitstream);

      for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {

          double sumY  = 0;
          double sumCb = 0;
          double sumCr = 0;

          for (uint8_t v = 0; v < 8; v++) {
            for (uint8_t u = 0; u < 8; u++) {
              uint8_t trans_pixel_index = v * 8 + u;

              double normU = (u == 0 ? 1/sqrt(2) : 1);
              double normV = (v == 0 ? 1/sqrt(2) : 1);

              double cosc1 = cos(((2 * x + 1) * u * JPEG2D_PI) / 16);
              double cosc2 = cos(((2 * y + 1) * v * JPEG2D_PI) / 16);

              sumY += block_Y[trans_pixel_index] * normU * normV * cosc1 * cosc2;
              sumCb += block_Cb[trans_pixel_index] * normU * normV * cosc1 * cosc2;
              sumCr += block_Cr[trans_pixel_index] * normU * normV * cosc1 * cosc2;
            }
          }

          uint8_t Y  = clamp((0.25 * sumY) + 128, 0.0, 255.0);
          uint8_t Cb = clamp((0.25 * sumCb) + 128, 0.0, 255.0);
          uint8_t Cr = clamp((0.25 * sumCr) + 128, 0.0, 255.0);

          uint64_t real_pixel_x = block_x * 8 + x;
          if (real_pixel_x >= width) {
            continue;
          }

          uint64_t real_pixel_y = block_y * 8 + y;
          if (real_pixel_y >= height) {
            continue;
          }

          uint64_t real_pixel_index = real_pixel_y * width + real_pixel_x;

          rgb_data[3 * real_pixel_index + 0] = clamp(Y + 1.402                            * (Cr - 128), 0.0, 255.0);
          rgb_data[3 * real_pixel_index + 1] = clamp(Y - 0.344136 * (Cb - 128) - 0.714136 * (Cr - 128), 0.0, 255.0);
          rgb_data[3 * real_pixel_index + 2] = clamp(Y + 1.772    * (Cb - 128)                        , 0.0, 255.0);
        }
      }
    }
  }

  if (!savePPM(output_filename, width, height, rgb_data)) {
    return false;
  }

  return true;
}

void readOneBlock(const vector<uint8_t> &counts_DC, const vector<uint8_t> &symbols_DC, const vector<uint8_t> &counts_AC, const vector<uint8_t> &symbols_AC, const QuantTable &quant_table, int16_t &prev_DC, Block<int16_t> &block, IBitstream &bitstream) {
  uint8_t index = 0;

  Block<int16_t> zigzag {};

  int16_t DC {decodeOnePair(counts_DC, symbols_DC, bitstream).amplitude};
  zigzag[index] = DC + prev_DC;
  prev_DC = zigzag[index];
  index++;

  while (true) {
    RunLengthPair pair = decodeOnePair(counts_AC, symbols_AC, bitstream);

    if ((pair.zeroes == 0) && (pair.amplitude == 0)) {
      break;
    }

    index += pair.zeroes;
    zigzag[index] = pair.amplitude;
    index++;
  }

  for (uint8_t i = 0; i < 64; i++) {
    block[i] = zigzag[zigzag_index_table[i]] * quant_table[i];
  }
}

void readHuffmanTable(vector<uint8_t> &counts, vector<uint8_t> &symbols, ifstream &stream) {
  counts.resize(stream.get());
  stream.read(reinterpret_cast<char *>(counts.data()), counts.size());

  symbols.resize(accumulate(counts.begin(), counts.end(), 0, std::plus<uint8_t>()));
  stream.read(reinterpret_cast<char *>(symbols.data()), symbols.size());
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
