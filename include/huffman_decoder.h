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

  bool load(std::ifstream &input);

  void print();

private:
  std::array<uint8_t, 16> m_counts;
  std::map<uint8_t, std::vector<bool>> m_codewords;
};

#endif
