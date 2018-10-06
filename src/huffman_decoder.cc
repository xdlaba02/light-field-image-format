#include "huffman_decoder.h"

#include <bitset>
#include <iostream>

HuffmanDecoder::HuffmanDecoder(): m_counts(), m_codewords() {}

HuffmanDecoder::~HuffmanDecoder() {}

bool HuffmanDecoder::load(std::ifstream &input) {
  input.read(reinterpret_cast<char *>(m_counts.data()), m_counts.size());

  uint16_t codeword = 0;

  for (uint8_t i = 0; i < 16; i++) {
    if (m_counts[i] > (1 << i)) {
      return false;
    }

    for (uint16_t j = 0; j < m_counts[i]; j++) {
      uint8_t key;
      input.read(reinterpret_cast<char *>(&key), 1);

      std::bitset<16> bits(codeword);
      for (uint8_t k = 0; k < i; k++) {
        m_codewords[key].push_back(bits[15 - k]);
      }

      codeword = ((codeword >> (16 - i)) + 1) << (16 - i);
    }
  }

  return true;
}

void HuffmanDecoder::print() {
  for (auto &pair: m_codewords) {
    std::cout << std::bitset<8>(pair.first) << ": ";
    for (auto &&bit: pair.second) {
      std::cout << bit;
    }
    std::cout << std::endl;
  }
}
