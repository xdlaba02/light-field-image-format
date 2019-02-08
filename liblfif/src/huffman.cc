/******************************************************************************\
* SOUBOR: huffman.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "huffman.h"

#include <numeric>
#include <algorithm>
#include <bitset>

void huffmanAddWeightAC(const RunLengthEncodedBlock &input, HuffmanWeights &weights) {
  for (size_t i = 1; i < input.size(); i++) {
    weights[huffmanSymbol(input[i])]++;
  }
}

void huffmanAddWeightDC(const RunLengthEncodedBlock &input, HuffmanWeights &weights) {
  weights[huffmanSymbol(input[0])]++;
}

HuffmanCodelengths generateHuffmanCodelengths(const HuffmanWeights &weights) {
  HuffmanCodelengths A {};

  A.reserve(weights.size());

  for (auto &pair: weights) {
    A.push_back({pair.second, pair.first});
  }

  sort(A.begin(), A.end());

  // SOURCE: http://hjemmesider.diku.dk/~jyrki/Paper/WADS95.pdf

  size_t n = A.size();

  unsigned long long s = 1;
  unsigned long long r = 1;

  for (unsigned long long t = 1; t <= n - 1; t++) {
    unsigned long long sum = 0;
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

  long long a  = 1;
  long long u  = 0;
  unsigned long long d = 0;
  unsigned long long t = n - 1;
  unsigned long long x = n;

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

HuffmanMap generateHuffmanMap(const HuffmanCodelengths &codelengths) {
  HuffmanMap map {};

  // TODO PROVE ME

  uint8_t prefix_ones = 0;

  uint8_t huffman_codeword {};
  for (auto &pair: codelengths) {
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

  return map;
}

HuffmanClass huffmanClass(RunLengthAmplitudeUnit amplitude) {
  if (amplitude < 0) {
    amplitude = -amplitude;
  }

  HuffmanClass huff_class = 0;
  while (amplitude > 0) {
    amplitude = amplitude >> 1;
    huff_class++;
  }

  return huff_class;
}

HuffmanSymbol huffmanSymbol(const RunLengthPair &pair) {
  return pair.zeroes << 4 | huffmanClass(pair.amplitude);
}

void encodeOneBlock(const RunLengthEncodedBlock &runlength, const HuffmanMap &huffmap_DC, const HuffmanMap &huffmap_AC, OBitstream &stream) {
  encodeOnePair(runlength[0], huffmap_DC, stream);
  for (size_t i = 1; i < runlength.size(); i++) {
    encodeOnePair(runlength[i], huffmap_AC, stream);
  }
}

void encodeOnePair(const RunLengthPair &pair, const HuffmanMap &map, OBitstream &stream) {
  HuffmanSymbol symbol = huffmanSymbol(pair);

  stream.write(map.at(symbol));

  HuffmanClass huff_class = huffmanClass(pair.amplitude);

  RunLengthAmplitudeUnit amplitude = pair.amplitude;
  if (amplitude < 0) {
    amplitude = -amplitude;
    amplitude = ~amplitude;
  }

  for (int8_t i = huff_class - 1; i >= 0; i--) {
    stream.writeBit((amplitude & (1 << i)) >> i);
  }
}

RunLengthEncodedBlock decodeOneBlock(const HuffmanTable &hufftable_DC, const HuffmanTable &hufftable_AC, IBitstream &bitstream) {
  RunLengthEncodedBlock output {};

  RunLengthPair pair {};

  output.push_back(decodeOnePair(hufftable_DC, bitstream));
  do {
    pair = decodeOnePair(hufftable_AC, bitstream);
    output.push_back(pair);
  } while((pair.zeroes != 0) || (pair.amplitude != 0));

  return output;
}

RunLengthPair decodeOnePair(const HuffmanTable &table, IBitstream &stream) {
  size_t symbol_index = decodeOneHuffmanSymbolIndex(table.counts, stream);
  RunLengthAmplitudeUnit amplitude = decodeOneAmplitude(table.symbols[symbol_index] & 0x0f, stream);
  return {static_cast<RunLengthZeroesCountUnit>(table.symbols[symbol_index] >> 4), amplitude};
}

size_t decodeOneHuffmanSymbolIndex(const vector<uint8_t> &counts, IBitstream &stream) {
  uint16_t code  = 0;
  uint16_t first = 0;
  uint16_t index = 0;
  uint16_t count = 0;

  for (size_t len = 1; len < counts.size(); len++) {
    code |= stream.readBit();
    count = counts[len];
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

RunLengthAmplitudeUnit decodeOneAmplitude(HuffmanSymbol length, IBitstream &stream) {
  RunLengthAmplitudeUnit amplitude = 0;

  if (length != 0) {
    for (HuffmanSymbol i = 0; i < length; i++) {
      amplitude <<= 1;
      amplitude |= stream.readBit();
    }

    if (amplitude < (1 << (length - 1))) {
      amplitude |= 0xffff << length;
      amplitude = ~amplitude;
      amplitude = -amplitude;
    }
  }

  return amplitude;
}

void writeHuffmanTable(const HuffmanCodelengths &codelengths, ofstream &stream) {
  uint8_t codelengths_cnt = codelengths.back().first + 1;
  stream.put(codelengths_cnt);

  auto it = codelengths.begin();
  for (uint8_t i = 0; i < codelengths_cnt; i++) {
    size_t leaves = 0;
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

HuffmanTable readHuffmanTable(ifstream &stream) {
  HuffmanTable table {};

  table.counts.resize(stream.get());
  stream.read(reinterpret_cast<char *>(table.counts.data()), table.counts.size());

  table.symbols.resize(accumulate(table.counts.begin(), table.counts.end(), 0, plus<uint8_t>()));
  stream.read(reinterpret_cast<char *>(table.symbols.data()), table.symbols.size());

  return table;
}
