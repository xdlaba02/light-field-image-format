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

RunLengthEncodedBlock decodeOneBlock(const HuffmanTable &hufftable_DC, const HuffmanTable &hufftable_AC, IBitstream &bitstream);
RunLengthPair decodeOnePair(const HuffmanTable &table, IBitstream &stream);
size_t decodeOneHuffmanSymbolIndex(const vector<uint8_t> &counts, IBitstream &stream);
RunLengthAmplitudeUnit decodeOneAmplitude(HuffmanSymbol length, IBitstream &stream);

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
inline TraversedBlock<D> runLenghtDecodeBlock(const RunLengthEncodedBlock &input) {
  TraversedBlock<D> output {};

  size_t pixel_index = 0;
  for (auto &pair: input) {
    pixel_index += pair.zeroes;
    output[pixel_index] = pair.amplitude;
    pixel_index++;
  }

  return output;
}

template<size_t D>
inline QuantizedBlock<D> detraverseBlock(const TraversedBlock<D> &input, const TraversalTable<D> &traversal_table) {
  QuantizedBlock<D> output {};

  for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
    output[pixel_index] = input[traversal_table[pixel_index]];
  }

  return output;
}

template<size_t D>
inline TransformedBlock<D> dequantizeBlock(const QuantizedBlock<D> &input, const QuantTable<D> &quant_table) {
  TransformedBlock<D> output {};

  for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
    output[pixel_index] = input[pixel_index] * quant_table[pixel_index];
  }

  return output;
}

template<size_t D>
inline YCbCrDataBlock<D> detransformBlock(const TransformedBlock<D> &input) {
  YCbCrDataBlock<D> output {};

  idct<D>([&](size_t index) -> DCTDataUnit { return input[index]; }, [&](size_t index) -> DCTDataUnit & { return output[index]; });

  return output;
}

template<size_t D>
inline YCbCrDataBlock<D> deshiftBlock(YCbCrDataBlock<D> input) {
  for (auto &pixel: input) {
    pixel += 128;
  }
  return input;
}

template<size_t D>
RGBDataBlock<D> YCbCrDataBlockToRGBDataBlock(const YCbCrDataBlock<D> &input_Y, const YCbCrDataBlock<D> &input_Cb, const YCbCrDataBlock<D> &input_Cr) {
  RGBDataBlock<D> output {};

  for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
    YCbCrDataUnit Y  = input_Y[pixel_index];
    YCbCrDataUnit Cb = input_Cb[pixel_index] - 128;
    YCbCrDataUnit Cr = input_Cr[pixel_index] - 128;

    output[pixel_index].r = clamp(Y +                      (1.402 * Cr), 0.0, 255.0);
    output[pixel_index].g = clamp(Y - (0.344136 * Cb) - (0.714136 * Cr), 0.0, 255.0);
    output[pixel_index].b = clamp(Y + (1.772    * Cb)                  , 0.0, 255.0);
  }

  return output;
}

template<size_t D>
struct putBlock {
  template <typename IF, typename OF>
  putBlock(IF &&input, size_t block, const size_t dims[D], OF &&output) {
    size_t blocks_x = 1;
    size_t size_x   = 1;

    for (size_t i = 0; i < D - 1; i++) {
      blocks_x *= ceil(dims[i]/8.0);
      size_x *= dims[i];
    }

    for (size_t pixel = 0; pixel < 8; pixel++) {
      size_t image = (block / blocks_x) * 8 + pixel;

      if (image >= dims[D-1]) {
        break;
      }

      auto inputF = [&](size_t pixel_index){
        return input(pixel * constpow(8, D-1) + pixel_index);
      };

      auto outputF = [&](size_t image_index)-> RGBDataUnit &{
        return output(image * size_x * 3 + image_index);
      };

      putBlock<D-1>(inputF, block % blocks_x, dims, outputF);
    }
  }
};

template<>
struct putBlock<1> {
  template <typename IF, typename OF>
  putBlock(IF &&input, size_t block, const size_t dims[1], OF &&output) {
    for (size_t pixel = 0; pixel < 8; pixel++) {
      size_t image = block * 8 + pixel;

      if (image >= dims[0]) {
        break;
      }

      output(image * 3 + 0) = input(pixel).r;
      output(image * 3 + 1) = input(pixel).g;
      output(image * 3 + 2) = input(pixel).b;
    }
  }
};

template<size_t D>
int LFIFDecompress(const char *input_file_name, RGBData &rgb_data, uint64_t img_dims[D], uint64_t &imgs_cnt) {
  ifstream input {};

  QuantTable<D> quant_table_luma   {};
  QuantTable<D> quant_table_chroma {};

  TraversalTable<D> traversal_table_luma   {};
  TraversalTable<D> traversal_table_chroma {};

  HuffmanTable hufftable_luma_DC   {};
  HuffmanTable hufftable_luma_AC   {};
  HuffmanTable hufftable_chroma_DC {};
  HuffmanTable hufftable_chroma_AC {};

  size_t blocks_cnt {};
  size_t pixels_cnt {};

  RunLengthAmplitudeUnit prev_Y  {};
  RunLengthAmplitudeUnit prev_Cb {};
  RunLengthAmplitudeUnit prev_Cr {};


  auto diffDecodeBlock = [&](RunLengthEncodedBlock input, RunLengthAmplitudeUnit &prev) {
    input[0].amplitude += prev;
    prev = input[0].amplitude;

    return input;
  };

  auto blockTo = [&](const RGBDataBlock<D> &input, size_t img, size_t block) {
    auto inputF = [&](size_t pixel_index){
      return input[pixel_index];
    };

    auto outputF = [&](size_t index) -> RGBDataUnit &{
      return rgb_data[img * pixels_cnt * 3 + index];
    };

    putBlock<D>(inputF, block, img_dims, outputF);
  };


  input.open(input_file_name);
  if (input.fail()) {
    return -1;
  }

  char magic_number[9] = "LFIF-#D\n";
  magic_number[5] = D + '0';

  if (!checkMagicNumber(magic_number, input)) {
    return -2;
  }

  for (size_t i = 0; i < D; i++) {
    img_dims[i] = readDimension(input);
  }

  imgs_cnt = readDimension(input);

  quant_table_luma = readQuantTable<D>(input);
  quant_table_chroma = readQuantTable<D>(input);

  traversal_table_luma = readTraversalTable<D>(input);
  traversal_table_chroma = readTraversalTable<D>(input);

  hufftable_luma_DC = readHuffmanTable(input);
  hufftable_luma_AC = readHuffmanTable(input);
  hufftable_chroma_DC = readHuffmanTable(input);
  hufftable_chroma_AC = readHuffmanTable(input);

  blocks_cnt = 1;
  pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    blocks_cnt *= ceil(img_dims[i]/8.);
    pixels_cnt *= img_dims[i];
  }

  rgb_data.resize(pixels_cnt * imgs_cnt * 3);

  prev_Y  = 0;
  prev_Cb = 0;
  prev_Cr = 0;

  IBitstream bitstream(input);

  for (size_t img = 0; img < imgs_cnt; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      YCbCrDataBlock<D> block_Y  = deshiftBlock<D>(detransformBlock<D>(dequantizeBlock<D>(detraverseBlock<D>(runLenghtDecodeBlock<D>(diffDecodeBlock(decodeOneBlock(hufftable_luma_DC,   hufftable_luma_AC,   bitstream), prev_Y)),  traversal_table_luma),   quant_table_luma)));
      YCbCrDataBlock<D> block_Cb = deshiftBlock<D>(detransformBlock<D>(dequantizeBlock<D>(detraverseBlock<D>(runLenghtDecodeBlock<D>(diffDecodeBlock(decodeOneBlock(hufftable_chroma_DC, hufftable_chroma_AC, bitstream), prev_Cb)), traversal_table_chroma), quant_table_chroma)));
      YCbCrDataBlock<D> block_Cr = deshiftBlock<D>(detransformBlock<D>(dequantizeBlock<D>(detraverseBlock<D>(runLenghtDecodeBlock<D>(diffDecodeBlock(decodeOneBlock(hufftable_chroma_DC, hufftable_chroma_AC, bitstream), prev_Cr)), traversal_table_chroma), quant_table_chroma)));

      blockTo(YCbCrDataBlockToRGBDataBlock<D>(block_Y, block_Cb, block_Cr), img, block);
    }
  }

  return 0;
}

#endif
