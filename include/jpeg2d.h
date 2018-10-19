/*******************************************************\
* SOUBOR: jpeg2d.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#ifndef JPEG2D_H
#define JPEG2D_H

#include "bitstream.h"

#include <cstdint>
#include <array>
#include <cmath>
#include <vector>
#include <map>

using namespace std;


/*******************************************************\
* TODO komentar
\*******************************************************/
template<typename T>

/*******************************************************\
* TODO komentar
\*******************************************************/
using Block = array<T, 64>;

/*******************************************************\
* TODO komentar
\*******************************************************/
using QuantTable = Block<uint8_t>;

/*******************************************************\
* TODO komentar
\*******************************************************/
using Codeword = vector<bool>;

/*******************************************************\
* TODO komentar
\*******************************************************/
const double JPEG2D_PI = 4 * atan(1);

/*******************************************************\
* TODO komentar
\*******************************************************/
const QuantTable universal_quant_table {
  16,11,10,16, 24, 40, 51, 61,
  12,12,14,19, 26, 58, 60, 55,
  14,13,16,24, 40, 57, 69, 56,
  14,17,22,29, 51, 87, 80, 62,
  18,22,37,56, 68,109,103, 77,
  24,35,55,64, 81,104,113, 92,
  49,64,78,87,103,121,120,101,
  72,92,95,98,112,100,103, 99
};

/*******************************************************\
* TODO komentar
\*******************************************************/
const Block<uint8_t> zigzag_index_table {
   0, 1, 5, 6,14,15,27,28,
   2, 4, 7,13,16,26,29,42,
   3, 8,12,17,25,30,41,43,
   9,11,18,24,31,40,44,53,
  10,19,23,32,39,45,52,54,
  20,22,33,38,46,51,55,60,
  21,34,37,47,50,56,59,61,
  35,36,48,49,57,58,62,63
};

/*******************************************************\
* TODO komentar
\*******************************************************/
struct RunLengthPair {
  uint8_t zeroes;
  int16_t amplitude;
};

/*#####################################################*/

/*******************************************************\
* TODO komentar
\*******************************************************/
bool PPM2JPEG2D(const char *input_filename, const char *output_filename, const uint8_t quality);

/*******************************************************\
* TODO komentar
\*******************************************************/
void runLengthDiffEncode(const Block<int16_t> &block_zigzag, int16_t &DC, int16_t &prev_DC, vector<RunLengthPair> &AC, map<uint8_t, uint64_t> &weights_DC, map<uint8_t, uint64_t> &weights_AC);

/*******************************************************\
* TODO komentar
\*******************************************************/
vector<pair<uint64_t, uint8_t>> huffmanGetCodelengths(const map<uint8_t, uint64_t> &weights);

/*******************************************************\
* TODO komentar
\*******************************************************/
map<uint8_t, Codeword> huffmanGenerateCodewords(const vector<pair<uint64_t, uint8_t>> &codelengths);

/*******************************************************\
* TODO komentar
\*******************************************************/
void writeHuffmanTable(const vector<pair<uint64_t, uint8_t>> &codelengths, ofstream &stream);

/*******************************************************\
* TODO komentar
\*******************************************************/
void encodeOnePair(const RunLengthPair &pair, const map<uint8_t, Codeword> &table, OBitstream &stream);

/*******************************************************\
* TODO komentar
\*******************************************************/
uint8_t huffmanClass(int16_t amplitude);

/*******************************************************\
* TODO komentar
\*******************************************************/
uint8_t huffmanSymbol(const RunLengthPair &pair);

/*#####################################################*/

/*******************************************************\
* TODO komentar
\*******************************************************/
bool JPEG2D2PPM(const char *input_filename, const char *output_filename);

/*******************************************************\
* TODO komentar
\*******************************************************/
void readOneBlock(const vector<uint8_t> &counts_DC, const vector<uint8_t> &symbols_DC, const vector<uint8_t> &counts_AC, const vector<uint8_t> &symbols_AC, const QuantTable &quant_table, int16_t &prev_DC, Block<int16_t> &block, IBitstream &bitstream);

/*******************************************************\
* TODO komentar
\*******************************************************/
void readHuffmanTable(vector<uint8_t> &counts, vector<uint8_t> &symbols, ifstream &stream);

/*******************************************************\
* TODO komentar
\*******************************************************/
RunLengthPair decodeOnePair(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream);

/*******************************************************\
* TODO komentar
\*******************************************************/
uint8_t decodeOneHuffmanSymbol(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream);

/*******************************************************\
* TODO komentar
\*******************************************************/
int16_t decodeOneAmplitude(uint8_t length, IBitstream &stream);

/*#####################################################*/

/*******************************************************\
* TODO komentar
\*******************************************************/
bool amIBigEndian();

/*******************************************************\
* TODO komentar
\*******************************************************/
uint64_t swapBytes(uint64_t v);

/*******************************************************\
* TODO komentar
\*******************************************************/
uint64_t toBigEndian(uint64_t v);

/*******************************************************\
* TODO komentar
\*******************************************************/
uint64_t fromBigEndian(uint64_t v);

#endif
