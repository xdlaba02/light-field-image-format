#ifndef HUFFMAN_DECODER_H
#define HUFFMAN_DECODER_H

#include <fstream>
#include <array>
#include <map>
#include <vector>

class HuffmanDecoder {
public:
  HuffmanDecoder();
  ~HuffmanDecoder();

  void load(std::ifstream &input);

  uint8_t decodeOne(std::vector<bool>::iterator &it) const;

  void print();

private:
  std::array<uint8_t, 16> m_counts;
  std::vector<uint8_t> m_symbols;
};

#endif
