/*******************************************************\
* SOUBOR: lfif_decoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#ifndef LFIF_DECODER_H
#define LFIF_DECODER_H

#include <iostream>

#include "lfif.h"
#include "dct.h"
#include "bitstream.h"

using namespace std;

HuffmanTable readHuffmanTable(ifstream &stream);

void decodeOneBlock(RunLengthEncodedBlock &pairs, const HuffmanTable &hufftable_DC, const HuffmanTable &hufftable_AC, IBitstream &bitstream);

RunLengthPair decodeOnePair(const HuffmanTable &table, IBitstream &stream);

size_t decodeOneHuffmanSymbolIndex(const vector<uint8_t> &counts, IBitstream &stream);

RunLengthAmplitudeUnit decodeOneAmplitude(HuffmanSymbol length, IBitstream &stream);

RunLengthEncodedImage diffDecodePairs(RunLengthEncodedImage runlengths);

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
inline vector<QuantizedBlock<D>> detraverseBlocks(const vector<TraversedBlock<D>> &blocks, const TraversalTable<D> &traversal_table) {
  vector<QuantizedBlock<D>> blocks_detraversed(blocks.size());

  for (size_t block_index = 0; block_index < blocks.size(); block_index++) {
    const TraversedBlock<D> &block            = blocks[block_index];
    QuantizedBlock<D>       &block_detraversed = blocks_detraversed[block_index];

    for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_detraversed[pixel_index] = block[traversal_table[pixel_index]];
    }
  }

  return blocks_detraversed;
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
struct convertFromBlocks {
  template <typename IF, typename OF>
  convertFromBlocks(IF &&input, const size_t dims[D], OF &&output) {
    size_t blocks_x = 1;
    size_t size_x   = 1;

    for (size_t i = 0; i < D - 1; i++) {
      blocks_x *= ceil(dims[i]/8.0);
      size_x *= dims[i];
    }

    size_t blocks = ceil(dims[D-1]/8.0);

    for (size_t block = 0; block < blocks; block++) {
      for (size_t pixel = 0; pixel < 8; pixel++) {
        size_t image = block * 8 + pixel;

        if (image >= dims[D-1]) {
          break;
        }

        auto inputF = [&](size_t block_index, size_t pixel_index){
          return input(block * blocks_x + block_index, pixel * constpow(8, D-1) + pixel_index);
        };

        auto outputF = [&](size_t image_index)-> YCbCrDataUnit &{
          return output(image * size_x + image_index);
        };

        convertFromBlocks<D-1>(inputF, dims, outputF);
      }
    }
  }
};

template<>
struct convertFromBlocks<1> {
  template <typename IF, typename OF>
  convertFromBlocks(IF &&input, const size_t dims[1], OF &&output) {
    size_t blocks = ceil(dims[0]/8.0);

    for (size_t block = 0; block < blocks; block++) {
      for (size_t pixel = 0; pixel < 8; pixel++) {
        size_t image = block * 8 + pixel;

        if (image >= dims[0]) {
          break;
        }

        output(image) = input(block, pixel);
      }
    }
  }
};

#endif
