/*******************************************************\
* SOUBOR: lfif_decoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#ifndef LFIF_DECODER_H
#define LFIF_DECODER_H

#include "lfif.h"
#include "dct.h"
#include "bitstream.h"

#include <cassert>
#include <iostream>
#include <iomanip>

using namespace std;

void readHuffmanTable(vector<uint8_t> &counts, vector<uint8_t> &symbols, ifstream &stream);

RunLengthPair decodeOnePair(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream);

void diffDecodePairs(vector<vector<RunLengthPair>> &runlengths);

uint8_t decodeOneHuffmanSymbol(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream);
int16_t decodeOneAmplitude(uint8_t length, IBitstream &stream);

vector<float> deshiftData(const vector<float> &data);

vector<uint8_t> YCbCrToRGB(const vector<float> &Y_data, const vector<float> &Cb_data, const vector<float> &Cr_data);

template<uint8_t D>
inline vector<Block<int16_t, D>> runLenghtDecodePairs(const vector<vector<RunLengthPair>> &runlengths) {
  vector<Block<int16_t, D>> blocks(runlengths.size());

  for (uint64_t block_index = 0; block_index < runlengths.size(); block_index++) {
    const vector<RunLengthPair> &vec   = runlengths[block_index];
    Block<int16_t, D>           &block = blocks[block_index];

    uint16_t pixel_index = 0;
    for (auto &pair: vec) {
      pixel_index += pair.zeroes;
      block[pixel_index] = pair.amplitude;
      pixel_index++;
    }
  }

  return blocks;
}

template<uint8_t D>
inline vector<Block<int16_t, D>> dezigzagBlocks(const vector<Block<int16_t, D>> &blocks, const TraversalTable<D> &traversal_table) {
  vector<Block<int16_t, D>> blocks_dezigzaged(blocks.size());

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    const Block<int16_t, D> &block            = blocks[block_index];
    Block<int16_t, D>       &block_dezigzaged = blocks_dezigzaged[block_index];

    for (uint16_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_dezigzaged[pixel_index] = block[traversal_table[pixel_index]];
    }
  }

  return blocks_dezigzaged;
}

template<uint8_t D>
inline vector<Block<int32_t, D>> dequantizeBlocks(const vector<Block<int16_t, D>> &blocks, const QuantTable<D> &quant_table) {
  vector<Block<int32_t, D>> blocks_dequantized(blocks.size());

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    const Block<int16_t, D> &block             = blocks[block_index];
    Block<int32_t, D>       &block_dequantized = blocks_dequantized[block_index];

    for (uint64_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_dequantized[pixel_index] = block[pixel_index] * quant_table[pixel_index];
    }
  }

  return blocks_dequantized;
}

template<uint8_t D>
inline vector<Block<float, D>> detransformBlocks(const vector<Block<int32_t, D>> &blocks) {
  vector<Block<float, D>> blocks_detransformed(blocks.size());

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    idct<D>([&](uint64_t index) -> float { return blocks[block_index][index]; }, [&](uint64_t index) -> float & { return blocks_detransformed[block_index][index]; });
  }

  return blocks_detransformed;
}

template<uint8_t D>
inline vector<float> convertFromBlocks(const Block<float, D> *blocks, const array<uint64_t, D> &dims) {
  array<uint64_t, D> block_dims {};
  uint64_t blocks_cnt = 1;
  uint64_t pixels_cnt = 1;

  for (uint8_t i = 0; i < D; i++) {
    block_dims[i] = ceil(dims[i]/8.0);
    blocks_cnt *= block_dims[i];
    pixels_cnt *= dims[i];
  }

  vector<float> data(pixels_cnt);

  for (uint64_t block_index = 0; block_index < blocks_cnt; block_index++) {
    const Block<float, D> &block = blocks[block_index];

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

      bool ok = true;

      for (uint8_t i = 0; i < D; i++) {
        if (image_coords[i] > dims[i] - 1) {
          ok = false;
          break;
        }
      }

      if (!ok) {
        continue;
      }

      uint64_t real_pixel_index = 0;
      uint64_t dim_acc = 1;

      for (uint8_t i = 0; i < D; i++) {
        real_pixel_index += image_coords[i] * dim_acc;
        dim_acc *= dims[i];
      }

      data[real_pixel_index] = block[pixel_index];
    }
  }

  return data;
}

#endif
