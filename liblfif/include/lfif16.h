/******************************************************************************\
* SOUBOR: lfif16.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF16_H
#define LFIF16_H

#include <cstdint>
#include <vector>

using namespace std;

struct RunLengthPair {
  size_t zeroes;
  int32_t amplitude;
};

struct HuffmanTable {
  vector<uint8_t> counts;
  vector<uint8_t> symbols;
};

int LFIFCompress16(const uint16_t *rgb_data, const uint64_t img_dims[2], uint8_t quality, const char *output_file_name);
int LFIFDecompress16(const char *input_file_name, vector<uint16_t> &rgb_data, uint64_t img_dims[2]);

#endif
