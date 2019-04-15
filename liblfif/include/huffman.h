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
#include <unordered_map>

using HuffmanSymbol     = uint16_t;
using HuffmanCodelength = HuffmanSymbol;
using HuffmanWeight     = uint64_t;
using HuffmanCodeword = std::vector<bool>;
using HuffmanWeights  = std::unordered_map<HuffmanSymbol, HuffmanWeight>;

class HuffmanEncoder {
public:
  HuffmanEncoder &generateFromWeights(const HuffmanWeights &huffman_weights);
  const HuffmanEncoder &writeToStream(std::ostream &stream) const;
  const HuffmanEncoder &encodeSymbolToStream(const HuffmanSymbol symbol, OBitstream &stream) const;

private:
  HuffmanEncoder &generateHuffmanCodelengths(const HuffmanWeights &huffman_weights);
  HuffmanEncoder &generateHuffmanMap();

  std::vector<std::pair<HuffmanWeight, HuffmanSymbol>> m_huffman_codelengths;
  std::unordered_map<HuffmanSymbol, HuffmanCodeword>  m_huffman_map;
};

class HuffmanDecoder {
public:
  HuffmanDecoder &readFromStream(std::istream &stream);
  HuffmanSymbol decodeSymbolFromStream(IBitstream &stream) const;

private:
  size_t decodeOneHuffmanSymbolIndex(IBitstream &stream) const;

  std::vector<HuffmanCodelength> m_huffman_counts;
  std::vector<HuffmanSymbol>     m_huffman_symbols;
};

#endif
