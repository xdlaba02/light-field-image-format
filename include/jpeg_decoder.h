/*******************************************************\
* SOUBOR: jpeg_decoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#ifndef JPEG_DECODER_H
#define JPEG_DECODER_H

#include "jpeg.h"
#include "bitstream.h"

#include <stdint.h>

#include <vector>

using namespace std;

void readHuffmanTable(vector<uint8_t> &counts, vector<uint8_t> &symbols, ifstream &stream);

RunLengthPair decodeOnePair(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream);

uint8_t decodeOneHuffmanSymbol(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream);

int16_t decodeOneAmplitude(uint8_t length, IBitstream &stream);

template<uint8_t D>
void readOneBlock(const vector<uint8_t> &counts_DC, const vector<uint8_t> &symbols_DC, const vector<uint8_t> &counts_AC, const vector<uint8_t> &symbols_AC, const QuantTable<D> &quant_table, int16_t &prev_DC, Block<int16_t, D> &block, IBitstream &bitstream) {
  uint16_t index = 0;

  Block<int16_t, D> zigzag_block {};

  int16_t DC = decodeOnePair(counts_DC, symbols_DC, bitstream).amplitude;
  zigzag_block[0] = DC + prev_DC;
  prev_DC = zigzag_block[0];
  index++;

  while (true) {
    RunLengthPair pair = decodeOnePair(counts_AC, symbols_AC, bitstream);

    if ((pair.zeroes == 0) && (pair.amplitude == 0)) {
      break;
    }

    index += pair.zeroes;
    zigzag_block[index] = pair.amplitude;
    index++;
  }

  for (uint16_t i = 0; i < block.size(); i++) {
    uint16_t zigzag_index = zigzagIndexTable<D>(i);
    int16_t value = zigzag_block[zigzag_index];
    block[i] = value * quant_table[i];
  }
}

#endif
