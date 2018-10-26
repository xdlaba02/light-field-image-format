/*******************************************************\
* SOUBOR: jpeg_decoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#ifndef JPEG_DECODER_H
#define JPEG_DECODER_H

#include "jpeg.h"
#include "bitstream.h"

#include <cstdint>
#include <vector>
#include <iostream>

using namespace std;

void readHuffmanTable(vector<uint8_t> &counts, vector<uint8_t> &symbols, ifstream &stream);

RunLengthPair decodeOnePair(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream);

uint8_t decodeOneHuffmanSymbol(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream);

int16_t decodeOneAmplitude(uint8_t length, IBitstream &stream);

void readOneBlock(const vector<uint8_t> &counts_DC, const vector<uint8_t> &counts_AC, const vector<uint8_t> &symbols_DC, const vector<uint8_t> &symbols_AC, int16_t &DC, vector<RunLengthPair> &AC, IBitstream &bitstream);

template<uint8_t D>
void runLengthDiffDecode(const int16_t DC, const vector<RunLengthPair> &AC, int16_t &prev_DC, Block<int16_t, D> &block) {
  block[0] = DC + prev_DC;
  prev_DC = block[0];

  uint16_t index = 1;

  for (uint64_t i = 0; i < AC.size() - 1; i++) {
    index += AC[i].zeroes;
    block[index] = AC[i].amplitude;
    index++;
  }
}

#endif
