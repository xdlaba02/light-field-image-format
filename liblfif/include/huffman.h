/******************************************************************************\
* SOUBOR: huffman.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <vector>
#include <map>

using namespace std;

using HuffmanSymbol = uint8_t;

using HuffmanCodeword = vector<bool>;
using HuffmanWeights = map<HuffmanSymbol, uint64_t>;
using HuffmanCodelengths = vector<pair<uint64_t, uint8_t>>;
using HuffmanMap = map<uint8_t, HuffmanCodeword>;

struct HuffmanTable {
  vector<uint8_t> counts;
  vector<uint8_t> symbols;
};

HuffmanCodelengths generateHuffmanCodelengths(const HuffmanWeights &weights);
HuffmanMap generateHuffmanMap(const HuffmanCodelengths &codelengths);

HuffmanTable readHuffmanTable(ifstream &stream);
void writeHuffmanTable(const HuffmanCodelengths &codelengths, ofstream &stream);

/*
HuffmanClass huffmanClass(QuantizedDataUnit amplitude);
void encodeOnePair(const RunLengthPair &pair, const HuffmanMap &map, OBitstream &stream);
RunLengthEncodedBlock decodeOneBlock(const HuffmanTable &hufftable_DC, const HuffmanTable &hufftable_AC, IBitstream &bitstream);
RunLengthPair decodeOnePair(const HuffmanTable &table, IBitstream &stream);
size_t decodeOneHuffmanSymbolIndex(const vector<uint8_t> &counts, IBitstream &stream);
QuantizedDataUnit decodeOneAmplitude(HuffmanSymbol length, IBitstream &stream);

*/
#endif
