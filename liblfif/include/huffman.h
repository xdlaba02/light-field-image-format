/******************************************************************************\
* SOUBOR: huffman.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "lfiftypes.h"
#include "bitstream.h"

void huffmanAddWeightAC(const RunLengthEncodedBlock &input, HuffmanWeights &weights);
void huffmanAddWeightDC(const RunLengthEncodedBlock &input, HuffmanWeights &weights);

HuffmanCodelengths generateHuffmanCodelengths(const HuffmanWeights &weights);
HuffmanMap generateHuffmanMap(const HuffmanCodelengths &codelengths);

HuffmanClass huffmanClass(RunLengthAmplitudeUnit amplitude);
HuffmanSymbol huffmanSymbol(const RunLengthPair &pair);

void encodeOneBlock(const RunLengthEncodedBlock &runlength, const HuffmanMap &huffmap_DC, const HuffmanMap &huffmap_AC, OBitstream &stream);
void encodeOnePair(const RunLengthPair &pair, const HuffmanMap &map, OBitstream &stream);

RunLengthEncodedBlock decodeOneBlock(const HuffmanTable &hufftable_DC, const HuffmanTable &hufftable_AC, IBitstream &bitstream);
RunLengthPair decodeOnePair(const HuffmanTable &table, IBitstream &stream);
size_t decodeOneHuffmanSymbolIndex(const vector<uint8_t> &counts, IBitstream &stream);
RunLengthAmplitudeUnit decodeOneAmplitude(HuffmanSymbol length, IBitstream &stream);

HuffmanTable readHuffmanTable(ifstream &stream);
void writeHuffmanTable(const HuffmanCodelengths &codelengths, ofstream &stream);

#endif
