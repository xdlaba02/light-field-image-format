/*******************************************************\
* SOUBOR: jpeg_encoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#ifndef JPEG_ENCODER_H
#define JPEG_ENCODER_H

#include "jpeg.h"
#include "bitstream.h"

#include <map>
#include <iostream>
#include <algorithm>

void huffmanGetWeightsDC(const vector<int16_t> &DC, map<uint8_t, uint64_t> &weights_DC);
void huffmanGetWeightsAC(const vector<vector<RunLengthPair>> &AC, map<uint8_t, uint64_t> &weights_AC);
vector<pair<uint64_t, uint8_t>> huffmanGetCodelengths(const map<uint8_t, uint64_t> &weights);
map<uint8_t, Codeword> huffmanGenerateCodewords(const vector<pair<uint64_t, uint8_t>> &codelengths);

void writeHuffmanTable(const vector<pair<uint64_t, uint8_t>> &codelengths, ofstream &stream);
void encodeOnePair(const RunLengthPair &pair, const map<uint8_t, Codeword> &table, OBitstream &stream);
void writeOneBlock(const int16_t DC, const vector<RunLengthPair> &AC, const map<uint8_t, Codeword> &huffcodes_DC, const map<uint8_t, Codeword> &huffcodes_AC, OBitstream &bitstream);

uint8_t huffmanClass(int16_t amplitude);
uint8_t huffmanSymbol(const RunLengthPair &pair);

template<uint8_t D>
void constructQuantTable(const uint8_t quality, QuantTable<D> &quant_table) {
  /* WIKI TABLE
  16,11,10,16, 24, 40, 51, 61,
  12,12,14,19, 26, 58, 60, 55,
  14,13,16,24, 40, 57, 69, 56,
  14,17,22,29, 51, 87, 80, 62,
  18,22,37,56, 68,109,103, 77,
  24,35,55,64, 81,104,113, 92,
  49,64,78,87,103,121,120,101,
  72,92,95,98,112,100,103, 99
  */

  float scale_coef = quality < 50 ? (5000.0 / quality) / 100 : (200.0 - 2 * quality) / 100;

  for (uint16_t i = 0; i < quant_table.size(); i++) {
    uint8_t x = i % 8;
    uint8_t y = i / 8;
    uint8_t xi = i / (8*8);
    uint8_t yi = i / (8*8*8);

    float radius = sqrt(x*x + y*y + xi*xi + yi*yi);

    float quant_value = (radius+1) * max(max(x, y), max(xi, yi));
    quant_value += 10;

    quant_value *= scale_coef;
    quant_table[i] = clamp(quant_value, 1.f, 128.f);
  }
}

template<uint8_t D>
void runLengthDiffEncode(const Block<int16_t, D> &block_zigzag, int16_t &DC, vector<RunLengthPair> &AC, int16_t &prev_DC) {
  uint16_t zeroes {};
  for (uint16_t pixel_index = 0; pixel_index < block_zigzag.size(); pixel_index++) {
    if (pixel_index == 0) {
      DC = block_zigzag[pixel_index] - prev_DC;
      prev_DC = block_zigzag[pixel_index];
    }
    else {
      if (block_zigzag[pixel_index] == 0) {
        zeroes++;
      }
      else {
        while (zeroes >= 16) {
          AC.push_back({15, 0});
          zeroes -= 16;
        }
        AC.push_back({static_cast<uint8_t>(zeroes), block_zigzag[pixel_index]});
        zeroes = 0;
      }
    }
  }
  AC.push_back({0, 0});
}

#endif
