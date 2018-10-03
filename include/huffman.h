#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "codec.h"

#include <bitset>
#include <map>
#include <iostream>

struct Codeword {
  Codeword(): codeword(0), length(0) {}
  uint16_t codeword;
  uint8_t length;

  Codeword operator<<(uint8_t val) {
    Codeword c;
    c.codeword = codeword << val;
    c.length = length + val;
    return c;
  }

  Codeword operator|(uint16_t val) {
    Codeword c;
    c.codeword = codeword | val;
    c.length = length;
    return c;
  }

  void print() {
    std::bitset<16> b(codeword);
    for (uint8_t i = 16 - length; i < 16; i++) {
      std::cout << static_cast<bool>(b[15 - i]);
    }
  }
};

class HuffmanTable {
public:
  HuffmanTable();
  ~HuffmanTable();

  void addKey(RLTuple &tuple);
  void constructTable();
  void printTable();

private:
  struct Node {
    Node(uint8_t v = 0): value(v), left(nullptr), right(nullptr) {}
    uint8_t value;
    Node *left;
    Node *right;
  };

  uint8_t key(RLTuple &tuple);
  uint8_t category(int16_t value);

  void consumeTree(Node *node, Codeword codeword);

  std::map<uint8_t, uint64_t> m_key_counts;
  std::map<uint8_t, Codeword> m_codewords;
};

#endif
