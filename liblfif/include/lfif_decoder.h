/******************************************************************************\
* SOUBOR: lfif_decoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_DECODER_H
#define LFIF_DECODER_H

#include "lfif.h"
#include "dct.h"
#include "bitstream.h"

using namespace std;

bool checkMagicNumber(const string &cmp, ifstream &input);

uint64_t readDimension(ifstream &input);

HuffmanTable readHuffmanTable(ifstream &stream);

void decodeOneBlock(RunLengthEncodedBlock &pairs, const HuffmanTable &hufftable_DC, const HuffmanTable &hufftable_AC, IBitstream &bitstream);

RunLengthPair decodeOnePair(const HuffmanTable &table, IBitstream &stream);

size_t decodeOneHuffmanSymbolIndex(const vector<uint8_t> &counts, IBitstream &stream);

RunLengthAmplitudeUnit decodeOneAmplitude(HuffmanSymbol length, IBitstream &stream);

RunLengthEncodedImage diffDecodePairs(RunLengthEncodedImage runlengths);

YCbCrData deshiftData(YCbCrData data);

RGBData YCbCrToRGB(const YCbCrData &Y_data, const YCbCrData &Cb_data, const YCbCrData &Cr_data);


template<size_t D>
QuantTable<D> readQuantTable(ifstream &input) {
  QuantTable<D> table {};
  input.read(reinterpret_cast<char *>(table.data()), table.size() * sizeof(QuantTable<0>::value_type));
  return table;
}

template<size_t D>
TraversalTable<D> readTraversalTable(ifstream &input) {
  TraversalTable<D> table {};
  input.read(reinterpret_cast<char *>(table.data()), table.size() * sizeof(TraversalTable<0>::value_type));
  return table;
}

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

template<size_t D>
bool LFIFDecompress(const char *input_file_name, RGBData &rgb_data, uint64_t img_dims[D], uint64_t &imgs_cnt) {
  ifstream input(input_file_name);
  if (input.fail()) {
    return false;
  }

  char magic_number[9] = "LFIF-#D\n";
  magic_number[5] = D + '0';

  if (!checkMagicNumber(magic_number, input)) {
    return false;
  }

  for (size_t i = 0; i < D; i++) {
    img_dims[i] = readDimension(input);
  }

  imgs_cnt = readDimension(input);

  QuantTable<D> quant_table_luma = readQuantTable<D>(input);
  QuantTable<D> quant_table_chroma = readQuantTable<D>(input);

  TraversalTable<D> traversal_table_luma = readTraversalTable<D>(input);
  TraversalTable<D> traversal_table_chroma = readTraversalTable<D>(input);

  HuffmanTable hufftable_luma_DC = readHuffmanTable(input);
  HuffmanTable hufftable_luma_AC = readHuffmanTable(input);
  HuffmanTable hufftable_chroma_DC = readHuffmanTable(input);
  HuffmanTable hufftable_chroma_AC = readHuffmanTable(input);

  size_t blocks_cnt = 1;
  size_t pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    blocks_cnt *= ceil(img_dims[i]/8.);
    pixels_cnt *= img_dims[i];
  }

  RunLengthEncodedImage pairs_Y(blocks_cnt * imgs_cnt);
  RunLengthEncodedImage pairs_Cb(blocks_cnt * imgs_cnt);
  RunLengthEncodedImage pairs_Cr(blocks_cnt * imgs_cnt);

  IBitstream bitstream(input);

  for (size_t i = 0; i < blocks_cnt * imgs_cnt; i++) {
    decodeOneBlock(pairs_Y[i],  hufftable_luma_DC, hufftable_luma_AC, bitstream);
    decodeOneBlock(pairs_Cb[i], hufftable_chroma_DC, hufftable_chroma_AC, bitstream);
    decodeOneBlock(pairs_Cr[i], hufftable_chroma_DC, hufftable_chroma_AC, bitstream);
  }

  auto deblockize = [&](const vector<YCbCrDataBlock<D>> &input) {
    YCbCrData output(pixels_cnt * imgs_cnt);

    for (size_t i = 0; i < imgs_cnt; i++) {
      auto inputF = [&](size_t block_index, size_t pixel_index){
        return input[(i * blocks_cnt) + block_index][pixel_index];
      };

      auto outputF = [&](size_t index) -> YCbCrDataUnit &{
        return output[(i * pixels_cnt) + index];
      };

      convertFromBlocks<D>(inputF, img_dims, outputF);
    }

    return output;
  };

  auto decode = [&](const RunLengthEncodedImage &input, const QuantTable<D> &quant_table, const TraversalTable<D> &traversal_table) {
    return deshiftData(deblockize(detransformBlocks<D>(dequantizeBlocks<D>(detraverseBlocks<D>(runLenghtDecodePairs<D>(diffDecodePairs(input)), traversal_table), quant_table))));
  };

  rgb_data = YCbCrToRGB(
    decode(pairs_Y, quant_table_luma, traversal_table_luma),
    decode(pairs_Cb, quant_table_chroma, traversal_table_chroma),
    decode(pairs_Cr, quant_table_chroma, traversal_table_chroma)
  );

  return true;
}

#endif
