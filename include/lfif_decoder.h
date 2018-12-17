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

using namespace std;

void readHuffmanTable(vector<uint8_t> &counts, vector<HuffmanSymbol> &symbols, ifstream &stream);

RunLengthPair decodeOnePair(const vector<uint8_t> &counts, const vector<HuffmanSymbol> &symbols, IBitstream &stream);

RunLengthEncodedImage diffDecodePairs(RunLengthEncodedImage runlengths);

HuffmanSymbol decodeOneHuffmanSymbol(const vector<uint8_t> &counts, const vector<HuffmanSymbol> &symbols, IBitstream &stream);
RunLengthAmplitudeUnit decodeOneAmplitude(HuffmanSymbol length, IBitstream &stream);

YCbCrData deshiftData(YCbCrData data);

RGBData YCbCrToRGB(const YCbCrData &Y_data, const YCbCrData &Cb_data, const YCbCrData &Cr_data);

template<size_t D>
inline vector<TraversedBlock<D>> runLenghtDecodePairs(const RunLengthEncodedImage &runlengths) {
  vector<TraversedBlock<D>> blocks(runlengths.size());

  for (size_t block_index = 0; block_index < runlengths.size(); block_index++) {
    const RunLengthEncodedBlock &vec   = runlengths[block_index];
    TraversedBlock<D>           &block = blocks[block_index];

    size_t pixel_index = 0;
    for (auto &pair: vec) {
      pixel_index += pair.zeroes;
      block[pixel_index] = pair.amplitude;
      pixel_index++;
    }
  }

  return blocks;
}

template<size_t D>
inline vector<QuantizedBlock<D>> dezigzagBlocks(const vector<TraversedBlock<D>> &blocks, const TraversalTable<D> &traversal_table) {
  vector<QuantizedBlock<D>> blocks_dezigzaged(blocks.size());

  for (size_t block_index = 0; block_index < blocks.size(); block_index++) {
    const TraversedBlock<D> &block            = blocks[block_index];
    QuantizedBlock<D>       &block_dezigzaged = blocks_dezigzaged[block_index];

    for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_dezigzaged[pixel_index] = block[traversal_table[pixel_index]];
    }
  }

  return blocks_dezigzaged;
}

template<size_t D>
inline vector<TransformedBlock<D>> dequantizeBlocks(const vector<QuantizedBlock<D>> &blocks, const QuantTable<D> &quant_table) {
  vector<TransformedBlock<D>> blocks_dequantized(blocks.size());

  for (size_t block_index = 0; block_index < blocks.size(); block_index++) {
    const QuantizedBlock<D> &block             = blocks[block_index];
    TransformedBlock<D>       &block_dequantized = blocks_dequantized[block_index];

    for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_dequantized[pixel_index] = block[pixel_index] * quant_table[pixel_index];
    }
  }

  return blocks_dequantized;
}

template<size_t D>
inline vector<YCbCrDataBlock<D>> detransformBlocks(const vector<TransformedBlock<D>> &blocks) {
  vector<YCbCrDataBlock<D>> blocks_detransformed(blocks.size());

  for (size_t block_index = 0; block_index < blocks.size(); block_index++) {
    idct<D>([&](size_t index) -> DCTDataUnit { return blocks[block_index][index]; }, [&](size_t index) -> DCTDataUnit & { return blocks_detransformed[block_index][index]; });
  }

  return blocks_detransformed;
}

template<size_t D>
inline YCbCrData convertFromBlocks(const YCbCrDataBlock<D> *blocks, const Dimensions<D> &dims) {
  Dimensions<D> block_dims {};
  size_t blocks_cnt = 1;
  size_t pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    block_dims[i] = ceil(dims[i]/8.0);
    blocks_cnt *= block_dims[i];
    pixels_cnt *= dims[i];
  }

  YCbCrData data(pixels_cnt);

  for (size_t block_index = 0; block_index < blocks_cnt; block_index++) {
    const YCbCrDataBlock<D> &block = blocks[block_index];

    Dimensions<D> block_coords {};

    size_t acc1 = 1;
    size_t acc2 = 1;

    for (size_t i = 0; i < D; i++) {
      acc1 *= block_dims[i];
      block_coords[i] = (block_index % acc1) / acc2;
      acc2 = acc1;
    }

    for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      array<size_t, D> pixel_coords {};

      for (size_t i = 0; i < D; i++) {
        pixel_coords[i] = (pixel_index % constpow(8, i+1)) / constpow(8, i);
      }

      Dimensions<D> image_coords {};

      for (size_t i = 0; i < D; i++) {
        image_coords[i] = block_coords[i] * 8 + pixel_coords[i];
      }

      bool ok = true;

      for (size_t i = 0; i < D; i++) {
        if (image_coords[i] > dims[i] - 1) {
          ok = false;
          break;
        }
      }

      if (!ok) {
        continue;
      }

      size_t real_pixel_index = 0;
      size_t dim_acc = 1;

      for (size_t i = 0; i < D; i++) {
        real_pixel_index += image_coords[i] * dim_acc;
        dim_acc *= dims[i];
      }

      data[real_pixel_index] = block[pixel_index];
    }
  }

  return data;
}

#endif
