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

vector<pair<uint64_t, uint8_t>> huffmanGetCodelengths(const map<uint8_t, uint64_t> &weights);

map<uint8_t, Codeword> huffmanGenerateCodewords(const vector<pair<uint64_t, uint8_t>> &codelengths);

void writeHuffmanTable(const vector<pair<uint64_t, uint8_t>> &codelengths, ofstream &stream);

void encodeOnePair(const RunLengthPair &pair, const map<uint8_t, Codeword> &table, OBitstream &stream);

uint8_t huffmanClass(int16_t amplitude);

uint8_t huffmanSymbol(const RunLengthPair &pair);

template<uint8_t D>
void scaleQuantTable(const uint8_t quality, QuantTable<D> &quant_table) {
  double scale_coef = quality < 50 ? (5000.0 / quality) / 100 : (200.0 - 2 * quality) / 100;

  for (uint16_t i = 0; i < quant_table.size(); i++) {
    uint8_t quant_value = universalQuantTable<D>(i) * scale_coef;
    quant_value = quant_value > 1 ? quant_value : 1;
    quant_table[i] = quant_value;
  }
}

template<uint8_t D>
void runLengthDiffEncode(const Block<int16_t, D> &block_zigzag, int16_t &DC, int16_t &prev_DC, vector<RunLengthPair> &AC, map<uint8_t, uint64_t> &weights_DC, map<uint8_t, uint64_t> &weights_AC) {
  uint8_t zeroes {};
  for (uint8_t pixel_index = 0; pixel_index < block_zigzag.size(); pixel_index++) {
    if (pixel_index == 0) {
      DC = block_zigzag[pixel_index] - prev_DC;
      prev_DC = block_zigzag[pixel_index];

      weights_DC[huffmanSymbol({0, DC})]++;
    }
    else {
      if (block_zigzag[pixel_index] == 0) {
        zeroes++;
      }
      else {
        while (zeroes >= 16) {
          AC.push_back({15, 0});
          zeroes -= 16;

          weights_AC[huffmanSymbol({15, 0})]++;
        }
        RunLengthPair pair {zeroes, block_zigzag[pixel_index]};

        AC.push_back(pair);
        zeroes = 0;

        weights_AC[huffmanSymbol(pair)]++;
      }
    }
  }
  AC.push_back({0, 0});

  weights_AC[huffmanSymbol({0, 0})]++;
}

#endif
