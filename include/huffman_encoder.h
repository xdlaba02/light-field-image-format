#ifndef HUFFMAN_ENCODER_H
#define HUFFMAN_ENCODER_H

//https://rosettacode.org/wiki/Huffman_coding#C.2B.2B
#include "huffman.h"

#include <vector>
#include <map>
#include <fstream>
#include <bitset>

class HuffmanEncoder {
public:
  HuffmanEncoder();
  ~HuffmanEncoder();

  void incrementKey(uint8_t key);
  void constructTable();
  void encode(const uint8_t key, std::vector<bool> &output);
  void writeTable(std::ofstream &stream);
  void print();

private:
  void treeToLevelOrder(const Node *node, const uint8_t depth);

  std::map<uint8_t, uint64_t> m_frequencies;
  std::array<std::vector<uint8_t>, 16> m_level_order_keys;
  std::map<uint8_t, HuffCode> m_codewords;
};

#endif
