/*******************************************************\
* SOUBOR: jpeg_encoder.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "jpeg_encoder.h"
#include "bitstream.h"

#include <bitset>
#include <algorithm>
#include <fstream>

using namespace std;

void huffmanGetWeightsDC(const vector<int16_t> &DC, map<uint8_t, uint64_t> &weights_DC) {
  for (int16_t amp: DC) {
    weights_DC[huffmanSymbol({0, amp})]++;
  }
}

void huffmanGetWeightsAC(const vector<vector<RunLengthPair>> &AC, map<uint8_t, uint64_t> &weights_AC) {
  for (auto &vec: AC) {
    for (auto &pair: vec) {
      weights_AC[huffmanSymbol({pair.zeroes, pair.amplitude})]++;
    }
  }
}

vector<pair<uint64_t, uint8_t>> huffmanGetCodelengths(const map<uint8_t, uint64_t> &weights) {
  vector<pair<uint64_t, uint8_t>> A {};

  A.reserve(weights.size());

  for (auto &pair: weights) {
    A.push_back({pair.second, pair.first});
  }

  sort(A.begin(), A.end());

  // SOURCE: http://hjemmesider.diku.dk/~jyrki/Paper/WADS95.pdf

  uint64_t n = A.size();

  uint64_t s = 1;
  uint64_t r = 1;

  for (uint64_t t = 1; t <= n - 1; t++) {
    uint64_t sum = 0;
    for (uint8_t i = 0; i < 2; i++) {
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

  return A;
}

map<uint8_t, Codeword> huffmanGenerateCodewords(const vector<pair<uint64_t, uint8_t>> &codelengths) {
  map<uint8_t, Codeword> codewords {};

  // TODO PROVE ME

  uint8_t prefix_ones = 0;

  uint8_t codeword {};
  for (auto &pair: codelengths) {
    codewords[pair.second];

    for (uint8_t i = 0; i < prefix_ones; i++) {
      codewords[pair.second].push_back(1);
    }

    uint8_t len = pair.first - prefix_ones;

    for (uint8_t k = 0; k < len; k++) {
      codewords[pair.second].push_back(bitset<8>(codeword)[7 - k]);
    }

    codeword = ((codeword >> (8 - len)) + 1) << (8 - len);
    while (codeword > 127) {
      prefix_ones++;
      codeword <<= 1;
    }
  }

  return codewords;
}

void writeHuffmanTable(const vector<pair<uint64_t, uint8_t>> &codelengths, ofstream &stream) {
  uint8_t codelengths_cnt = codelengths.back().first + 1;
  stream.put(codelengths_cnt);

  auto it = codelengths.begin();
  for (uint8_t i = 0; i < codelengths_cnt; i++) {
    uint16_t leaves = 0;
    while ((it < codelengths.end()) && ((*it).first == i)) {
      leaves++;
      it++;
    }
    stream.put(leaves);
  }

  for (auto &pair: codelengths) {
    stream.put(pair.second);
  }
}

void encodeOnePair(const RunLengthPair &pair, const map<uint8_t, Codeword> &table, OBitstream &stream) {
  uint8_t huff_class = huffmanClass(pair.amplitude);
  uint8_t symbol     = huffmanSymbol(pair);

  stream.write(table.at(symbol));

  int16_t amplitude = pair.amplitude;
  if (amplitude < 0) {
    amplitude = -amplitude;
    amplitude = ~amplitude;
  }

  for (int8_t i = huff_class - 1; i >= 0; i--) {
    stream.writeBit((amplitude & (1 << i)) >> i);
  }
}

void writeOneBlock(const int16_t DC, const vector<RunLengthPair> &AC, const map<uint8_t, Codeword> &huffcodes_DC, const map<uint8_t, Codeword> &huffcodes_AC, OBitstream &bitstream) {
  encodeOnePair({0, DC}, huffcodes_DC, bitstream);
  for (auto &pair: AC) {
    encodeOnePair(pair, huffcodes_AC, bitstream);
  }
}


uint8_t huffmanClass(int16_t amplitude) {
  if (amplitude < 0) {
    amplitude = -amplitude;
  }

  uint8_t huff_class = 0;
  while (amplitude > 0) {
    amplitude = amplitude >> 1;
    huff_class++;
  }

  return huff_class;
}

uint8_t huffmanSymbol(const RunLengthPair &pair) {
  return pair.zeroes << 4 | huffmanClass(pair.amplitude);
}
