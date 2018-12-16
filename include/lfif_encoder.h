/*******************************************************\
* SOUBOR: lfif_encoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#ifndef LFIF_ENCODER_H
#define LFIF_ENCODER_H

#include "lfif.h"
#include "dct.h"
#include "bitstream.h"

#include <map>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <sstream>

void huffmanGetWeights(const vector<vector<RunLengthPair>> &pairvecs, map<uint8_t, uint64_t> &weights_AC, map<uint8_t, uint64_t> &weights_DC);

vector<pair<uint64_t, uint8_t>> huffmanGetCodelengths(const map<uint8_t, uint64_t> &weights);
map<uint8_t, Codeword> huffmanGenerateCodewords(const vector<pair<uint64_t, uint8_t>> &codelengths);

void writeHuffmanTable(const vector<pair<uint64_t, uint8_t>> &codelengths, ofstream &stream);

void encodePairs(const vector<vector<RunLengthPair>> &pairvecs, const map<uint8_t, Codeword> &huffcodes_AC, const map<uint8_t, Codeword> &huffcodes_DC, OBitstream &bitstream);
void encodeOnePair(const RunLengthPair &pair, const map<uint8_t, Codeword> &table, OBitstream &stream);

uint8_t huffmanClass(int16_t amplitude);
uint8_t huffmanSymbol(const RunLengthPair &pair);

inline float RGBtoY(uint8_t R, uint8_t G, uint8_t B) {
  return          0.299 * R +    0.587 * G +    0.114 * B;
}

inline float RGBtoCb(uint8_t R, uint8_t G, uint8_t B) {
  return 128 - 0.168736 * R - 0.331264 * G +      0.5 * B;
}

inline float RGBtoCr(uint8_t R, uint8_t G, uint8_t B) {
  return 128 +      0.5 * R - 0.418688 * G - 0.081312 * B;
}

template<uint8_t D>
constexpr QuantTable<D> baseQuantTable() {
  QuantTable<D> quant_table {};

  for (uint64_t i = 0; i < quant_table.size(); i++) {
    uint64_t sum = 0;
    uint64_t max = 0;
    for (uint8_t j = 1; j <= D; j++) {
      uint8_t coord = (i % constpow(8, j)) / constpow(8, j-1);
      sum += coord * coord;
      if (coord > max) {
        max = coord;
      }
    }
    quant_table[i] = clamp(((sqrt(sum)+1) * max) + 10, 1., 255.);
  }

  return quant_table;
}

template<uint8_t D>
inline QuantTable<D> scaleQuantTable(const QuantTable<D> &quant_table, const uint8_t quality) {
  QuantTable<D> output {};
  float scale_coef = quality < 50 ? (5000.0 / quality) / 100 : (200.0 - 2 * quality) / 100;
  for (uint64_t i = 0; i < quant_table.size(); i++) {
    output[i] = clamp<float>(quant_table[i] * scale_coef, 1, 255);
  }
  return output;
}

template <typename F>
inline vector<float> convertRGB(const vector<uint8_t> &rgb_data, F &&function) {
  uint64_t pixels_cnt = rgb_data.size()/3;

  vector<float> data(pixels_cnt);

  for (uint64_t pixel_index = 0; pixel_index < pixels_cnt; pixel_index++) {
    uint8_t R = rgb_data[3*pixel_index + 0];
    uint8_t G = rgb_data[3*pixel_index + 1];
    uint8_t B = rgb_data[3*pixel_index + 2];

    data[pixel_index] = function(R, G, B);
  }

  return data;
}

void shiftData(vector<float> &data) {
  for (auto &pixel: data) {
    pixel -= 128;
  }
}

template<uint8_t D>
inline vector<Block<float, D>> convertToBlocks(const vector<float> &data, const array<uint64_t, D> &dims) {
  array<uint64_t, D> block_dims {};
  uint64_t blocks_cnt = 1;

  for (uint8_t i = 0; i < D; i++) {
    block_dims[i] = ceil(dims[i]/8.0);
    blocks_cnt *= block_dims[i];
  }

  vector<Block<float, D>> blocks(blocks_cnt);

  for (uint64_t block_index = 0; block_index < blocks_cnt; block_index++) {
    Block<float, D> &block = blocks[block_index];

    array<uint64_t, D> block_coords {};

    uint64_t acc1 = 1;
    uint64_t acc2 = 1;

    for (uint8_t i = 0; i < D; i++) {
      acc1 *= block_dims[i];
      block_coords[i] = (block_index % acc1) / acc2;
      acc2 = acc1;
    }

    for (uint64_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      array<uint8_t, D> pixel_coords {};

      for (uint8_t i = 0; i < D; i++) {
        pixel_coords[i] = (pixel_index % constpow(8, i+1)) / constpow(8, i);
      }

      array<uint64_t, D> image_coords {};

      for (uint8_t i = 0; i < D; i++) {
        image_coords[i] = block_coords[i] * 8 + pixel_coords[i];
      }

      /*******************************************************\
      * V pripade, ze velikost obrazku neni nasobek osmi,
        se blok doplni z krajnich pixelu.
      \*******************************************************/
      for (uint8_t i = 0; i < D; i++) {
        if (image_coords[i] > dims[i] - 1) {
          image_coords[i] = dims[i] - 1;
        }
      }

      uint64_t input_pixel_index = 0;
      uint64_t dim_acc = 1;

      for (uint8_t i = 0; i < D; i++) {
        input_pixel_index += image_coords[i] * dim_acc;
        dim_acc *= dims[i];
      }

      block[pixel_index] = data[input_pixel_index];
    }
  }

  return blocks;
}

template<uint8_t D>
vector<Block<float, D>> transformBlocks(const vector<Block<float, D>> &blocks) {
  vector<Block<float, D>> blocks_transformed(blocks.size());

  auto dct = [](const Block<float, D> &block, Block<float, D> &block_transformed){
    fdct<D>([&](uint64_t index) -> float { return block[index];}, [&](uint64_t index) -> float & { return block_transformed[index]; });
  };

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    dct(blocks[block_index], blocks_transformed[block_index]);
  }

  return blocks_transformed;
}

template<uint8_t D>
inline vector<Block<int16_t, D>> quantizeBlocks(const vector<Block<float, D>> &blocks, const QuantTable<D> &quant_table) {
  vector<Block<int16_t, D>> blocks_quantized(blocks.size());

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    const Block<float, D> &block            = blocks[block_index];
    Block<int16_t, D>     &block_quantized  = blocks_quantized[block_index];

    for (uint64_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_quantized[pixel_index] = block[pixel_index] / quant_table[pixel_index];
    }
  }

  return blocks_quantized;
}

template<uint8_t D>
inline TraversalTable<D> constructTraversalTableByAvg(vector<Block<int16_t, D>> &blocks, vector<Block<int16_t, D>> &blocks_Cb, vector<Block<int16_t, D>> &blocks_Cr) {
  TraversalTable<D>                     traversal_table {};
  Block<pair<uint64_t, uint16_t>, D> srt          {};

  for (uint64_t i = 0; i < constpow(8, D); i++) {
    srt[i].second = i;
  }

  for (uint64_t b = 0; b < blocks.size(); b++) {
    for (uint64_t i = 0; i < constpow(8, D); i++) {
      srt[i].first += abs(blocks[b][i]) + abs(blocks_Cb[b][i]) + abs(blocks_Cr[b][i]);
    }
  }

  stable_sort(srt.rbegin(), srt.rend());

  for (uint64_t i = 0; i < constpow(8, D); i++) {
    traversal_table[srt[i].second] = i;
  }

  return traversal_table;
}

template<uint8_t D>
TraversalTable<D> constructTraversalTableByRadius() {
  TraversalTable<D>                     traversal_table {};
  Block<pair<uint64_t, uint16_t>, D> srt          {};

  for (uint64_t i = 0; i < constpow(8, D); i++) {
    for (uint8_t j = 1; j <= D; j++) {
      uint8_t coord = (i % constpow(8, j)) / constpow(8, j-1);
      srt[i].first += coord * coord;
    }
    srt[i].second = i;
  }

  stable_sort(srt.begin(), srt.end());

  for (uint64_t i = 0; i < constpow(8, D); i++) {
    traversal_table[srt[i].second] = i;
  }

  return traversal_table;
}

template<uint8_t D>
inline TraversalTable<D> constructTraversalTableByDiagonals() {
  TraversalTable<D>                     traversal_table {};
  Block<pair<uint64_t, uint16_t>, D> srt          {};

  for (uint64_t i = 0; i < constpow(8, D); i++) {
    for (uint8_t j = 1; j <= D; j++) {
      srt[i].first += (i % constpow(8, j)) / constpow(8, j-1);
    }
    srt[i].second = i;
  }

  stable_sort(srt.begin(), srt.end());

  for (uint64_t i = 0; i < constpow(8, D); i++) {
    traversal_table[srt[i].second] = i;
  }

  return traversal_table;
}

template<uint8_t D>
inline vector<Block<int16_t, D>> traverseBlocks(const vector<Block<int16_t, D>> &blocks, const TraversalTable<D> &traversal_table) {
  vector<Block<int16_t, D>> blocks_zigzaged(blocks.size());

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    const Block<int16_t, D> &block          = blocks[block_index];
    Block<int16_t, D>       &block_zigzaged = blocks_zigzaged[block_index];

    for (uint64_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_zigzaged[traversal_table[pixel_index]] = block[pixel_index];
    }
  }

  return blocks_zigzaged;
}

template<uint8_t D>
inline vector<vector<RunLengthPair>> runLenghtDiffEncodeBlocks(const vector<Block<int16_t, D>> &blocks) {
  vector<vector<RunLengthPair>> runlengths(blocks.size());

  int16_t prev_DC = 0;

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    const Block<int16_t, D> &block     = blocks[block_index];
    vector<RunLengthPair>   &runlength = runlengths[block_index];

    runlength.push_back({0, static_cast<int16_t>(block[0] - prev_DC)});
    prev_DC = block[0];

    uint64_t zeroes = 0;
    for (uint64_t pixel_index = 1; pixel_index < constpow(8, D); pixel_index++) {
      if (block[pixel_index] == 0) {
        zeroes++;
      }
      else {
        while (zeroes >= 16) {
          runlength.push_back({15, 0});
          zeroes -= 16;
        }
        runlength.push_back({static_cast<uint8_t>(zeroes), block[pixel_index]});
        zeroes = 0;
      }
    }

    runlength.push_back({0, 0});
  }

  return runlengths;
}


template<uint8_t D>
inline bool RGBtoLFIF(const vector<float> &data, const array<uint64_t, D> &dimensions, const uint8_t quality) {
  static_assert(D <= 4);


  vector<Block<int16_t, D>> blocks_zigzag  = traverseBlocks<D>(blocks_quantized,  traversal_table);



  return runLenghtDiffEncodeBlocks<D>(blocks_zigzag);
}

#endif
