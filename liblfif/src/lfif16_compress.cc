/******************************************************************************\
* SOUBOR: lfif16_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "lfif16.h"
#include "bitstream.h"

#include <cstddef>
#include <cmath>

#include <array>
#include <map>
#include <bitset>
#include <algorithm>
#include <fstream>

using namespace std;

inline void getBlock(const uint16_t *rgb_data, const uint64_t img_dims[2], size_t block, uint16_t out_block[64*3]) {
  size_t blocks_x = ceil(img_dims[0]/8.0);
  size_t size_x   = img_dims[0];

  for (size_t pixel_y = 0; pixel_y < 8; pixel_y++) {
    size_t image_y = (block / blocks_x) * 8 + pixel_y;

    if (image_y >= img_dims[1]) {
      image_y = img_dims[1] - 1;
    }

    for (size_t pixel_x = 0; pixel_x < 8; pixel_x++) {
      size_t image_x = block % blocks_x * 8 + pixel_x;

      if (image_x >= img_dims[0]) {
        image_x = img_dims[0] - 1;
      }

      out_block[(pixel_y * 8 + pixel_x) * 3 + 0] = rgb_data[(image_y * size_x + image_x) * 3 + 0];
      out_block[(pixel_y * 8 + pixel_x) * 3 + 1] = rgb_data[(image_y * size_x + image_x) * 3 + 1];
      out_block[(pixel_y * 8 + pixel_x) * 3 + 2] = rgb_data[(image_y * size_x + image_x) * 3 + 2];
    }
  }
}

inline double RGBToY(uint16_t R, uint16_t G, uint16_t B) {
  return (0.299 * R) + (0.587 * G) + (0.114 * B);
}

inline double RGBToCb(uint16_t R, uint16_t G, uint16_t B) {
  return 32768 - (0.168736 * R) - (0.331264 * G) + (0.5 * B);
}

inline double RGBToCr(uint16_t R, uint16_t G, uint16_t B) {
  return 32768 + (0.5 * R) - (0.418688 * G) - (0.081312 * B);
}

inline void convertRGBDataBlock(const uint16_t input[64*3], double (*f)(uint16_t, uint16_t, uint16_t), double output[64]) {
  for (size_t i = 0; i < 64; i++) {
    uint16_t R = input[i * 3 + 0];
    uint16_t G = input[i * 3 + 1];
    uint16_t B = input[i * 3 + 2];
    output[i] = f(R, G, B);
  }
}

inline void shiftBlock(double input[64]) {
  for (size_t i = 0; i < 64; i++) {
    input[i] -= 32768;
  }
}

inline void scaleQuantTable(uint16_t quant_table[64], const uint8_t quality) {
  float scale_coef = quality < 50 ? (5000.0 / quality) / 100 : (200.0 - 2 * quality) / 100;
  for (size_t i = 0; i < 64; i++) {
    quant_table[i] = clamp<float>(quant_table[i] * scale_coef, 1, 255);
  }
}

inline void quantizeBlock(const double input[64], const uint16_t quant_table[64], int32_t output[64]) {
  for (size_t pixel_index = 0; pixel_index < 64; pixel_index++) {
    output[pixel_index] = input[pixel_index] / quant_table[pixel_index];
  }
}

inline void addToReference(const int32_t input[64], double reference[64]) {
  for (size_t i = 0; i < 64; i++) {
    reference[i] += abs(input[i]);
  }
}

inline void constructTraversalTableByReference(const double ref_block[64], uint8_t traversal_table[64]) {
  array<pair<double, size_t>, 64> srt {};

  for (size_t i = 0; i < 64; i++) {
    srt[i].first += abs(ref_block[i]);
    srt[i].second = i;
  }

  stable_sort(srt.rbegin(), srt.rend());

  for (size_t i = 0; i < 64; i++) {
    traversal_table[srt[i].second] = i;
  }
}

inline void traverseBlock(int32_t input[64], const uint8_t traversal_table[64]) {
  int32_t tmp[64] {};
  for (size_t i = 0; i < 64; i++) {
    tmp[traversal_table[i]] = input[i];
  }
  for (size_t i = 0; i < 64; i++) {
    input[i] = tmp[i];
  }
}

inline void diffEncodeDC(int32_t &input, int32_t &prev) {
  int32_t curr = input;
  input -= prev;
  prev = curr;
}

inline vector<RunLengthPair> runLenghtEncodeBlock(const int32_t input[64]) {
  vector<RunLengthPair> output {};

  output.push_back({0, input[0]});

  size_t zeroes = 0;
  for (size_t pixel_index = 1; pixel_index < 64; pixel_index++) {
    if (input[pixel_index] == 0) {
      zeroes++;
    }
    else {
      while (zeroes >= 8) {
        output.push_back({7, 0});
        zeroes -= 8;
      }
      output.push_back({zeroes, input[pixel_index]});
      zeroes = 0;
    }
  }

  output.push_back({0, 0});
  return output;
}

uint8_t huffmanClass(int32_t amplitude) {
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
  return pair.zeroes << 5 | huffmanClass(pair.amplitude);
}

inline void huffmanAddWeights(const vector<RunLengthPair> &input, map<uint8_t, uint64_t> weights[2]) {
  weights[0][huffmanSymbol(input[0])]++;
  for (size_t i = 1; i < input.size(); i++) {
    weights[1][huffmanSymbol(input[i])]++;
  }
}

inline vector<pair<uint64_t, uint8_t>> generateHuffmanCodelengths(const map<uint8_t, uint64_t> &weights) {
  vector<pair<uint64_t, uint8_t>> A {};

  A.reserve(weights.size());

  for (auto &pair: weights) {
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

  return A;
}

inline map<uint8_t, vector<bool>> generateHuffmanMap(const vector<pair<uint64_t, uint8_t>> &codelengths) {
  map<uint8_t, vector<bool>> map {};

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

inline void writeDimension(uint64_t dim, ofstream &output) {
  uint64_t raw = htobe64(dim);
  output.write(reinterpret_cast<char *>(&raw), sizeof(raw));
}

inline void writeQuantTable(const uint16_t quant_table[64], ofstream &output) {
  for (size_t i = 0; i < 64; i++) {
    uint16_t raw = htobe16(quant_table[i]);
    output.write(reinterpret_cast<const char *>(&raw), sizeof(raw));
  }
}

inline void writeTraversalTable(const uint8_t traversal_table[64], ofstream &output) {
  output.write(reinterpret_cast<const char *>(traversal_table), 64);
}

inline void writeHuffmanTable(const vector<pair<uint64_t, uint8_t>> &codelengths, ofstream &stream) {
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

void encodeOnePair(const RunLengthPair &pair, const map<uint8_t, vector<bool>> &map, OBitstream &stream) {
  stream.write(map.at(huffmanSymbol(pair)));

  uint8_t huff_class = huffmanClass(pair.amplitude);

  int32_t amplitude = pair.amplitude;
  if (amplitude < 0) {
    amplitude = -amplitude;
    amplitude = ~amplitude;
  }

  for (int8_t i = huff_class - 1; i >= 0; i--) {
    stream.writeBit((amplitude & (1 << i)) >> i);
  }
}

void encodeOneBlock(const vector<RunLengthPair> &runlength, const map<uint8_t, vector<bool>> huffmaps[2], OBitstream &stream) {
  encodeOnePair(runlength[0], huffmaps[0], stream);
  for (size_t i = 1; i < runlength.size(); i++) {
    encodeOnePair(runlength[i], huffmaps[1], stream);
  }
}

int LFIFCompress16(const uint16_t *rgb_data, const uint64_t img_dims[2], uint8_t quality, const char *output_file_name) {
  size_t blocks_cnt {};

  uint16_t quant_table_luma[64] {
    16, 11, 10, 16, 124, 140, 151, 161,
    12, 12, 14, 19, 126, 158, 160, 155,
    14, 13, 16, 24, 140, 157, 169, 156,
    14, 17, 22, 29, 151, 187, 180, 162,
    18, 22, 37, 56, 168, 109, 103, 177,
    24, 35, 55, 64, 181, 104, 113, 192,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 199,
  };

  uint16_t quant_table_chroma[64] {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
  };

  for (size_t i = 0; i < 64; i++) {
    quant_table_luma[i] *= 655535/255;
    quant_table_chroma[i] *= 655535/255;
  }

  double reference_block_luma[64]   {};
  double reference_block_chroma[64] {};

  uint8_t traversal_table_luma[64]   {};
  uint8_t traversal_table_chroma[64] {};

  int32_t previous_DCs[3] {};

  map<uint8_t, uint64_t> weights_luma[2]   {};
  map<uint8_t, uint64_t> weights_chroma[2] {};

  vector<pair<uint64_t, uint8_t>> codelengths_luma[2]   {};
  vector<pair<uint64_t, uint8_t>> codelengths_chroma[2] {};

  map<uint8_t, vector<bool>> huffmap_luma[2]   {};
  map<uint8_t, vector<bool>> huffmap_chroma[2] {};

  double (*colorspace_convertors[3]) (uint16_t, uint16_t, uint16_t) {};
  uint16_t *quant_tables[3] {};
  double *reference_blocks[3] {};
  uint8_t *traversal_tables[3] {};
  map<uint8_t, uint64_t> *huffman_weights[3] {};
  map<uint8_t, vector<bool>> *huffmaps[3] {};

  blocks_cnt = ceil(img_dims[0]/8.) * ceil(img_dims[1]/8.);

  colorspace_convertors[0] = RGBToY;
  colorspace_convertors[1] = RGBToCb;
  colorspace_convertors[2] = RGBToCr;

  quant_tables[0] = quant_table_luma;
  quant_tables[1] = quant_table_chroma;
  quant_tables[2] = quant_table_chroma;

  reference_blocks[0] = reference_block_luma;
  reference_blocks[1] = reference_block_chroma;
  reference_blocks[2] = reference_block_chroma;

  traversal_tables[0] = traversal_table_luma;
  traversal_tables[1] = traversal_table_chroma;
  traversal_tables[2] = traversal_table_chroma;

  huffman_weights[0] = weights_luma;
  huffman_weights[1] = weights_chroma;
  huffman_weights[2] = weights_chroma;

  huffmaps[0] = huffmap_luma;
  huffmaps[1] = huffmap_chroma;
  huffmaps[2] = huffmap_chroma;

  scaleQuantTable(quant_table_luma, quality);
  scaleQuantTable(quant_table_chroma, quality);

  for (size_t block = 0; block < blocks_cnt; block++) {
    uint16_t block_rgb[64*3] {};

    getBlock(rgb_data, img_dims, block, block_rgb);

    for (size_t channel = 0; channel < 3; channel++) {
      double   block_raw[64]   {};
      double   block_coefs[64] {};
      int32_t  block_quant[64] {};

      convertRGBDataBlock(block_rgb, colorspace_convertors[channel], block_raw);
      shiftBlock(block_raw);
      fdct<2>([&](size_t index) { return block_raw[index]; }, [&](size_t index) -> double & { return block_coefs[index]; });
      quantizeBlock(block_coefs, quant_tables[channel], block_quant);
      addToReference(block_quant, reference_blocks[channel]);
    }
  }

  constructTraversalTableByReference(reference_block_luma, traversal_table_luma);
  constructTraversalTableByReference(reference_block_chroma, traversal_table_chroma);

  previous_DCs[0] = 0;
  previous_DCs[1] = 0;
  previous_DCs[2] = 0;

  for (size_t block = 0; block < blocks_cnt; block++) {
    uint16_t block_rgb[64*3] {};

    getBlock(rgb_data, img_dims, block, block_rgb);

    for (size_t channel = 0; channel < 3; channel++) {
      double   block_raw[64]   {};
      double   block_coefs[64] {};
      int32_t  block_quant[64] {};

      convertRGBDataBlock(block_rgb, colorspace_convertors[channel], block_raw);
      shiftBlock(block_raw);
      fdct<2>([&](size_t index) { return block_raw[index]; }, [&](size_t index) -> double & { return block_coefs[index]; });
      quantizeBlock(block_coefs, quant_tables[channel], block_quant);
      traverseBlock(block_quant, traversal_tables[channel]);
      diffEncodeDC(block_quant[0], previous_DCs[channel]);
      huffmanAddWeights(runLenghtEncodeBlock(block_quant), huffman_weights[channel]);
    }
  }

  codelengths_luma[0]   = generateHuffmanCodelengths(weights_luma[0]);
  codelengths_luma[1]   = generateHuffmanCodelengths(weights_luma[1]);
  codelengths_chroma[0] = generateHuffmanCodelengths(weights_chroma[0]);
  codelengths_chroma[1] = generateHuffmanCodelengths(weights_chroma[1]);

  huffmap_luma[0]   = generateHuffmanMap(codelengths_luma[0]);
  huffmap_luma[1]   = generateHuffmanMap(codelengths_luma[1]);
  huffmap_chroma[0] = generateHuffmanMap(codelengths_chroma[0]);
  huffmap_chroma[1] = generateHuffmanMap(codelengths_chroma[1]);

  size_t last_slash_pos = string(output_file_name).find_last_of('/');

  if (last_slash_pos != string::npos) {
    string command("mkdir -p " + string(output_file_name).substr(0, last_slash_pos));
    system(command.c_str());
  }

  ofstream output(output_file_name);
  if (output.fail()) {
    return -1;
  }

  output.write("LFIF-16\n", 8);

  writeDimension(img_dims[0], output);
  writeDimension(img_dims[1], output);

  writeQuantTable(quant_table_luma, output);
  writeQuantTable(quant_table_chroma, output);

  writeTraversalTable(traversal_table_luma, output);
  writeTraversalTable(traversal_table_chroma, output);

  writeHuffmanTable(codelengths_luma[0], output);
  writeHuffmanTable(codelengths_luma[1], output);
  writeHuffmanTable(codelengths_chroma[0], output);
  writeHuffmanTable(codelengths_chroma[1], output);

  OBitstream bitstream(output);

  previous_DCs[0] = 0;
  previous_DCs[1] = 0;
  previous_DCs[2] = 0;

  for (size_t block = 0; block < blocks_cnt; block++) {
    uint16_t block_rgb[64*3] {};

    getBlock(rgb_data, img_dims, block, block_rgb);

    for (size_t channel = 0; channel < 3; channel++) {
      double   block_raw[64]   {};
      double   block_coefs[64] {};
      int32_t  block_quant[64] {};

      convertRGBDataBlock(block_rgb, colorspace_convertors[channel], block_raw);
      shiftBlock(block_raw);
      fdct<2>([&](size_t index) { return block_raw[index]; }, [&](size_t index) -> double & { return block_coefs[index]; });
      quantizeBlock(block_coefs, quant_tables[channel], block_quant);
      traverseBlock(block_quant,  traversal_tables[channel]);
      diffEncodeDC(block_quant[0], previous_DCs[channel]);
      encodeOneBlock(runLenghtEncodeBlock(block_quant), huffmaps[channel], bitstream);
    }
  }

  bitstream.flush();

  return 0;

}
