/******************************************************************************\
* SOUBOR: huffman.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef HUFFMAN_H
#define HUFFMAN_H

class IBitstream;
class OBitstream;

#include <iosfwd>

#include <vector>
#include <map>

using HuffmanSymbol   = uint8_t;
using HuffmanCodeword = std::vector<bool>;
using HuffmanWeights  = std::map<HuffmanSymbol, uint64_t>;

class HuffmanEncoder {
public:
  HuffmanEncoder &generateFromWeights(const HuffmanWeights &huffman_weights);
  HuffmanEncoder &writeToStream(std::ofstream &stream);
  HuffmanEncoder &encodeSymbolToStream(HuffmanSymbol symbol, OBitstream &stream);

public: //FIXME
  HuffmanEncoder &generateHuffmanCodelengths(const HuffmanWeights &huffman_weights);
  HuffmanEncoder &generateHuffmanMap();

  std::vector<std::pair<uint64_t, HuffmanSymbol>> m_huffman_codelengths;
  std::map<HuffmanSymbol, HuffmanCodeword>        m_huffman_map;
};

class HuffmanDecoder {
public:
  HuffmanDecoder &readFromStream(std::ifstream &stream);
  HuffmanSymbol decodeSymbolFromStream(IBitstream &stream);

public: // FIXME
  size_t decodeOneHuffmanSymbolIndex(IBitstream &stream);

  std::vector<uint8_t>       m_huffman_counts;
  std::vector<HuffmanSymbol> m_huffman_symbols;
};

#endif
