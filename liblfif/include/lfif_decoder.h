/******************************************************************************\
* SOUBOR: lfif_decoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_DECODER_H
#define LFIF_DECODER_H

#include "lfiftypes.h"
#include "block.h"
#include "qtable.h"
#include "traversal.h"
#include "huffman.h"
#include "shift.h"
#include "dct.h"
#include "runlength.h"
#include "colorspace.h"

#include <fstream>

inline bool checkMagicNumber(const string &cmp, ifstream &input) {
  char magic_number[9] {};
  input.read(magic_number, 8);
  return string(magic_number) == cmp;
}

inline uint64_t readDimension(ifstream &input) {
  uint64_t raw {};
  input.read(reinterpret_cast<char *>(&raw), sizeof(uint64_t));
  return be64toh(raw);
}

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
