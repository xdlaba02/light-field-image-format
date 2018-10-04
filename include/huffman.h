#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "codec.h"

#include <bitset>
#include <map>
#include <iostream>

struct BitField {
  BitField(): value(0), length(0) {}
  uint16_t value;
  uint8_t length;

  BitField operator<<(uint8_t val) {
    BitField b;
    b.value = value << val;
    b.length = length + val;
    return b;
  }

  BitField operator|(uint16_t val) {
    BitField b;
    b.value = value | val;
    b.length = length;
    return b;
  }

  void print() {
    std::bitset<16> b(value);
    for (uint8_t i = 16 - length; i < 16; i++) {
      std::cout << static_cast<bool>(b[15 - i]);
    }
  }
};

class HuffmanTable {
public:
  HuffmanTable();
  ~HuffmanTable();

  void addKey(const RLTuple &tuple);
  void constructTable();
  BitField getCodeword(const RLTuple &tuple);
  static BitField getEncodedValue(const RLTuple &tuple);

  void printTable();

private:
  struct Node {
    Node(uint8_t v = 0): value(v), left(nullptr), right(nullptr) {}
    uint8_t value;
    Node *left;
    Node *right;
  };

  static uint8_t key(const RLTuple &tuple);
  static uint8_t category(int16_t value);

  void consumeTree(Node *node, BitField BitField);

  std::map<uint8_t, uint64_t> m_key_counts;
  std::map<uint8_t, BitField> m_codewords;
};

#endif
