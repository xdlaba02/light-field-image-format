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
struct convertFromBlocks {
  template <typename IF, typename OF>
  convertFromBlocks(IF &&input, const size_t dims[D], OF &&output) {
    size_t blocks = ceil(dims[D-1]/8.0);

    for (size_t block = 0; block < blocks; block++) {
      for (size_t pixel = 0; pixel < 8; pixel++) {
        size_t image = block * 8 + pixel;

        if (image >= dims[D-1]) {
          break;
        }

        convertFromBlocks<D-1>([&](size_t block_index, size_t pixel_index){ return input(block * blocks + block_index, pixel * constpow(8, D-1) + pixel_index); }, dims, [&](size_t image_index)-> YCbCrDataUnit &{ return output(image * dims[D-1] + image_index); });
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
