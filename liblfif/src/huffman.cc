/******************************************************************************\
* SOUBOR: huffman.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "huffman.h"
#include "bitstream.h"

#include <algorithm>
#include <bitset>
#include <numeric>
#include <fstream>

using namespace std;

HuffmanEncoder &HuffmanEncoder::generateFromWeights(const HuffmanWeights &huffman_weights) {
  generateHuffmanCodelengths(huffman_weights);
  generateHuffmanMap();

  return *this;
}

HuffmanEncoder &HuffmanEncoder::writeToStream(ofstream &stream) {
  uint8_t codelengths_cnt = m_huffman_codelengths.back().first + 1;
  stream.put(codelengths_cnt);

  auto it = m_huffman_codelengths.begin();
  for (uint8_t i = 0; i < codelengths_cnt; i++) {
    size_t leaves = 0;
    while ((it < m_huffman_codelengths.end()) && ((*it).first == i)) {
      leaves++;
      it++;
    }
    stream.put(leaves);
  }

  for (auto &pair: m_huffman_codelengths) {
    stream.put(pair.second);
  }

  return *this;
}

HuffmanEncoder &HuffmanEncoder::encodeSymbolToStream(HuffmanSymbol symbol, OBitstream &stream) {
  stream.write(m_huffman_map.at(symbol));
  return *this;
}

HuffmanEncoder &HuffmanEncoder::generateHuffmanCodelengths(const HuffmanWeights &huffman_weights) {
  vector<pair<uint64_t, HuffmanSymbol>> A {};

  A.reserve(huffman_weights.size());

  for (auto &pair: huffman_weights) {
    A.push_back({pair.second, pair.first});
  }

  sort(A.begin(), A.end());

  // SOURCE: http://hjemmesider.diku.dk/~jyrki/Paper/WADS95.pdf

  size_t n = A.size();

  uint64_t s = 1;
  uint64_t r = 1;

  for (uint64_t t = 1; t <= n - 1; t++) {
    uint64_t sum = 0;
    for (size_t i = 0; i < 2; i++) {
      if ((s > n) || ((r < t) && (A[r-1].first < A[s-1].first))) {
        sum += A[r-1].first;
        A[r-1].first = t;
        r++;
      }
      else {
        sum += A[s-1].first;
        s++;
      }
    }
    A[t-1].first = sum;
  }

  if (n >= 2) {
    A[n - 2].first = 0;
  }

  for (int64_t t = n - 2; t >= 1; t--) {
    A[t-1].first = A[A[t-1].first-1].first + 1;
  }

  int64_t a  = 1;
  int64_t u  = 0;
  uint64_t d = 0;
  uint64_t t = n - 1;
  uint64_t x = n;

  while (a > 0) {
    while ((t >= 1) && (A[t-1].first == d)) {
      u++;
      t--;
    }
    while (a > u) {
      A[x-1].first = d;
      x--;
      a--;
    }
    a = 2 * u;
    d++;
    u = 0;
  }

  sort(A.begin(), A.end());

  m_huffman_codelengths = A;

  return *this;
}

HuffmanEncoder &HuffmanEncoder::generateHuffmanMap() {
  map<HuffmanSymbol, HuffmanCodeword> map {};

  // TODO PROVE ME

  uint8_t prefix_ones = 0;

  uint8_t huffman_codeword {};
  for (auto &pair: m_huffman_codelengths) {
    map[pair.second];

    for (uint8_t i = 0; i < prefix_ones; i++) {
      map[pair.second].push_back(1);
    }

    uint8_t len = pair.first - prefix_ones;

    for (uint8_t k = 0; k < len; k++) {
      map[pair.second].push_back(bitset<8>(huffman_codeword)[7 - k]);
    }

    huffman_codeword = ((huffman_codeword >> (8 - len)) + 1) << (8 - len);
    while (huffman_codeword > 127) {
      prefix_ones++;
      huffman_codeword <<= 1;
    }
  }

  m_huffman_map = map;

  return *this;
}

HuffmanDecoder &HuffmanDecoder::readFromStream(ifstream &stream) {
  m_huffman_counts.resize(stream.get());
  stream.read(reinterpret_cast<char *>(m_huffman_counts.data()), m_huffman_counts.size());

  m_huffman_symbols.resize(accumulate(m_huffman_counts.begin(), m_huffman_counts.end(), 0, plus<uint8_t>()));
  stream.read(reinterpret_cast<char *>(m_huffman_symbols.data()), m_huffman_symbols.size());

  return *this;
}

HuffmanSymbol HuffmanDecoder::decodeSymbolFromStream(IBitstream &stream) {
  return m_huffman_symbols[decodeOneHuffmanSymbolIndex(stream)];
}

size_t HuffmanDecoder::decodeOneHuffmanSymbolIndex(IBitstream &stream) {
  uint16_t code  = 0;
  uint16_t first = 0;
  uint16_t index = 0;
  uint16_t count = 0;

  for (size_t len = 1; len < m_huffman_counts.size(); len++) {
    code |= stream.readBit();
    count = m_huffman_counts[len];
    if (code - count < first) {
      return index + (code - first);
    }
    index += count;
    first += count;
    first <<= 1;
    code <<= 1;
  }

  return 0;
}
