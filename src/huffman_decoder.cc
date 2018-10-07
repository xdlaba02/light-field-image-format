#include "huffman_decoder.h"

#include <bitset>
#include <iostream>
#include <numeric>

HuffmanDecoder::HuffmanDecoder():
  m_counts(),
  m_symbols() {}

HuffmanDecoder::~HuffmanDecoder() {}

void HuffmanDecoder::load(std::ifstream &input) {
  input.read(reinterpret_cast<char *>(m_counts.data()), m_counts.size());
  m_symbols.resize(std::accumulate(m_counts.begin(), m_counts.end(), 0, std::plus<uint8_t>()));
  input.read(reinterpret_cast<char *>(m_symbols.data()), m_symbols.size());
}

uint8_t HuffmanDecoder::decodeOne(std::vector<bool>::iterator &it) const {
  uint8_t code = 0;
  uint8_t first = 0;
  uint8_t index = 0;
  uint8_t count = 0;

  for (uint8_t len = 1; len < 16; len++) {
    code |= *it++;
    count = m_counts[len];
    if (code - count < first) {
      return m_symbols[index + (code - first)];
    }
    index += count;
    first += count;
    first <<= 1;
    code <<= 1;
  }
  return 0;
}

void HuffmanDecoder::print() {
  std::vector<uint8_t>::iterator it = m_symbols.begin();
  uint16_t codeword = 0;
  for (uint8_t i = 0; i < 16; i++) {
    for (uint8_t j = 0; j < m_counts[i]; j++) {
      std::cout << std::bitset<8>(*it) << ": ";
      it++;
      for (uint8_t k = 0; k < i; k++) {
        std::cout << std::bitset<16>(codeword)[15 - k];
      }
      std::cout << std::endl;
      codeword = ((codeword >> (16 - i)) + 1) << (16 - i);
    }
  }
}
