/**
* @file huffman.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for Huffman encoding and decoding.
*/

#ifndef HUFFMAN_H
#define HUFFMAN_H

class IBitstream;
class OBitstream;

#include <iosfwd>
#include <vector>
#include <unordered_map>

using HuffmanSymbol      = uint16_t;          /**< @brief Size of Huffman symbol is determined by this type.*/
using HuffmanWeight      = uint64_t;          /**< @brief Type used for counting occurences of symbols.*/
using HuffmanCodelength  = HuffmanSymbol;     /**< @brief Type used for determining code lenght. The maximum code length in Huffman encoding is equal to maximum value of encoded symbol.*/
using HuffmanCodeword    = std::vector<bool>; /**< @brief Type used for variable length codeword.*/

/**
* @brief Map used for counting occurences of symbols for optimal Huffman encoding.
*/
using HuffmanWeights     = std::unordered_map<HuffmanSymbol, HuffmanWeight>;

/**
* @brief Map used for fast encoding symbols to codewords.
*/
using HuffmanMap         = std::unordered_map<HuffmanSymbol, HuffmanCodeword>;

/**
* @brief Class used for encoding fixed size symbols to variable length bit sequences.
*/
class HuffmanEncoder {
public:

  /**
  * @brief Method which initializes encoder by count of occurences of each symbol.
  * @param huffman_weights Occurences of symbols.
  * @return Reference to itself (for chaining).
  */
  HuffmanEncoder &generateFromWeights(const HuffmanWeights &huffman_weights);

  /**
  * @brief Method which writes the encoder data to stream.
  * @param stream Stream to which the data should be written.
  * @return Reference to itself (for chaining).
  */
  const HuffmanEncoder &writeToStream(std::ostream &stream) const;

  /**
  * @brief Method which encodes fixed size symbol to bitstream.
  * @param symbol Symbol to be encoded.
  * @param stream Bitstream to which symbol will be encoded.
  * @return Reference to itself (for chaining).
  */
  const HuffmanEncoder &encodeSymbolToStream(const HuffmanSymbol symbol, OBitstream &stream) const;

private:
  HuffmanEncoder &generateHuffmanCodelengths(const HuffmanWeights &huffman_weights);
  HuffmanEncoder &generateHuffmanMap();

  std::vector<std::pair<HuffmanWeight, HuffmanSymbol>> m_huffman_codelengths;
  HuffmanMap                                           m_huffman_map;
};

/**
* @brief Class used for decoding variable length bit sequences to fixed size symbols.
*/
class HuffmanDecoder {
public:

  /**
  * @brief Method which initializes decoder from data written by HuffmanEncoder::writeToStream.
  * @param stream Stream from which the metadata will be read.
  * @return Reference to itself (for chaining).
  */
  HuffmanDecoder &readFromStream(std::istream &stream);

  /**
  * @brief Method which decodes one symbol from bitstream.
  * @param stream Stream from which the symbol will be read.
  * @return Reference to itself (for chaining).
  */
  HuffmanSymbol decodeSymbolFromStream(IBitstream &stream) const;

private:
  size_t decodeOneHuffmanSymbolIndex(IBitstream &stream) const;

  std::vector<HuffmanCodelength> m_huffman_counts;
  std::vector<HuffmanSymbol>     m_huffman_symbols;
};

#endif
