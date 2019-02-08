/******************************************************************************\
* SOUBOR: lfif_encoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_ENCODER_H
#define LFIF_ENCODER_H

#include "lfiftypes.h"
#include "block.h"
#include "qtable.h"
#include "dct.h"
#include "shift.h"
#include "colorspace.h"
#include "traversal.h"
#include "runlength.h"
#include "huffman.h"

#include <fstream>

using namespace std;

inline void writeMagicNumber(const char *number, ofstream &output) {
  output.write(number, 8);
}

inline void writeDimension(uint64_t dim, ofstream &output) {
  uint64_t raw = htobe64(dim);
  output.write(reinterpret_cast<char *>(&raw),  sizeof(raw));
}

template<size_t D>
int LFIFCompress(const RGBData &rgb_data, const uint64_t img_dims[D], uint64_t imgs_cnt, uint8_t quality, const char *output_file_name) {
  size_t blocks_cnt {};
  size_t pixels_cnt {};

  QuantTable<D> quant_table_luma   {};
  QuantTable<D> quant_table_chroma {};

  RefereceBlock<D> reference_block_luma   {};
  RefereceBlock<D> reference_block_chroma {};

  TraversalTable<D> traversal_table_luma   {};
  TraversalTable<D> traversal_table_chroma {};

  HuffmanWeights weights_luma_AC   {};
  HuffmanWeights weights_luma_DC   {};
  HuffmanWeights weights_chroma_AC {};
  HuffmanWeights weights_chroma_DC {};

  HuffmanCodelengths codelengths_luma_DC   {};
  HuffmanCodelengths codelengths_luma_AC   {};
  HuffmanCodelengths codelengths_chroma_DC {};
  HuffmanCodelengths codelengths_chroma_AC {};

  RunLengthAmplitudeUnit prev_Y  {};
  RunLengthAmplitudeUnit prev_Cb {};
  RunLengthAmplitudeUnit prev_Cr {};

  HuffmanMap huffmap_luma_DC   {};
  HuffmanMap huffmap_luma_AC   {};
  HuffmanMap huffmap_chroma_DC {};
  HuffmanMap huffmap_chroma_AC {};

  auto blockAt = [&](size_t img, size_t block) {
    RGBDataBlock<D> output {};

    auto inputF = [&](size_t index) {
      return rgb_data[img * pixels_cnt * 3 + index];
    };

    auto outputF = [&](size_t index) -> RGBDataPixel &{
      return output[index];
    };

    getBlock<D>(inputF, block, img_dims, outputF);

    return output;
  };

  auto diffEncodeBlock = [&](RunLengthEncodedBlock input, RunLengthAmplitudeUnit &prev) {
    RunLengthAmplitudeUnit curr = input[0].amplitude;
    input[0].amplitude -= prev;
    prev = curr;

    return input;
  };

  blocks_cnt = 1;
  pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    blocks_cnt *= ceil(img_dims[i]/8.);
    pixels_cnt *= img_dims[i];
  }

  quant_table_luma   = scaleQuantTable<D>(baseQuantTableLuma<D>(), quality);
  quant_table_chroma = scaleQuantTable<D>(baseQuantTableChroma<D>(), quality);

  for (size_t img = 0; img < imgs_cnt; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      RGBDataBlock<D> rgb_block = blockAt(img, block);

      QuantizedBlock<D> quantized_Y  = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoY))),  quant_table_luma);
      QuantizedBlock<D> quantized_Cb = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoCb))), quant_table_chroma);
      QuantizedBlock<D> quantized_Cr = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoCr))), quant_table_chroma);

      addToReference<D>(quantized_Y,  reference_block_luma);
      addToReference<D>(quantized_Cb, reference_block_chroma);
      addToReference<D>(quantized_Cr, reference_block_chroma);
    }
  }

  traversal_table_luma   = constructTraversalTableByReference<D>(reference_block_luma);
  traversal_table_chroma = constructTraversalTableByReference<D>(reference_block_chroma);

  prev_Y  = 0;
  prev_Cb = 0;
  prev_Cr = 0;

  for (size_t img = 0; img < imgs_cnt; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      RGBDataBlock<D> rgb_block = blockAt(img, block);

      QuantizedBlock<D> quantized_Y  = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoY))),  quant_table_luma);
      QuantizedBlock<D> quantized_Cb = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoCb))), quant_table_chroma);
      QuantizedBlock<D> quantized_Cr = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoCr))), quant_table_chroma);

      RunLengthEncodedBlock runlength_Y  = diffEncodeBlock(runLenghtEncodeBlock<D>(traverseBlock<D>(quantized_Y,  traversal_table_luma)),   prev_Y);
      RunLengthEncodedBlock runlength_Cb = diffEncodeBlock(runLenghtEncodeBlock<D>(traverseBlock<D>(quantized_Cb, traversal_table_chroma)), prev_Cb);
      RunLengthEncodedBlock runlength_Cr = diffEncodeBlock(runLenghtEncodeBlock<D>(traverseBlock<D>(quantized_Cr, traversal_table_chroma)), prev_Cr);

      huffmanAddWeightAC(runlength_Y,  weights_luma_AC);
      huffmanAddWeightDC(runlength_Y,  weights_luma_DC);

      huffmanAddWeightAC(runlength_Cb, weights_chroma_AC);
      huffmanAddWeightAC(runlength_Cr, weights_chroma_AC);
      huffmanAddWeightDC(runlength_Cb, weights_chroma_DC);
      huffmanAddWeightDC(runlength_Cr, weights_chroma_DC);
    }
  }

  codelengths_luma_DC   = generateHuffmanCodelengths(weights_luma_DC);
  codelengths_luma_AC   = generateHuffmanCodelengths(weights_luma_AC);
  codelengths_chroma_DC = generateHuffmanCodelengths(weights_chroma_DC);
  codelengths_chroma_AC = generateHuffmanCodelengths(weights_chroma_AC);

  size_t last_slash_pos = string(output_file_name).find_last_of('/');
  string command("mkdir -p " + string(output_file_name).substr(0, last_slash_pos));
  system(command.c_str());

  ofstream output(output_file_name);
  if (output.fail()) {
    return -1;
  }

  char magic_number[9] = "LFIF-#D\n";
  magic_number[5] = D + '0';

  writeMagicNumber(magic_number, output);

  for (size_t i = 0; i < D; i++) {
    writeDimension(img_dims[i], output);
  }

  writeDimension(imgs_cnt, output);

  writeQuantTable<D>(quant_table_luma, output);
  writeQuantTable<D>(quant_table_chroma, output);

  writeTraversalTable<D>(traversal_table_luma, output);
  writeTraversalTable<D>(traversal_table_chroma, output);

  writeHuffmanTable(codelengths_luma_DC, output);
  writeHuffmanTable(codelengths_luma_AC, output);
  writeHuffmanTable(codelengths_chroma_DC, output);
  writeHuffmanTable(codelengths_chroma_AC, output);

  huffmap_luma_DC   = generateHuffmanMap(codelengths_luma_DC);
  huffmap_luma_AC   = generateHuffmanMap(codelengths_luma_AC);
  huffmap_chroma_DC = generateHuffmanMap(codelengths_chroma_DC);
  huffmap_chroma_AC = generateHuffmanMap(codelengths_chroma_AC);

  prev_Y  = 0;
  prev_Cb = 0;
  prev_Cr = 0;

  OBitstream bitstream(output);

  for (size_t img = 0; img < imgs_cnt; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      RGBDataBlock<D> rgb_block = blockAt(img, block);

      QuantizedBlock<D> quantized_Y  = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoY))), quant_table_luma);
      QuantizedBlock<D> quantized_Cb = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoCb))), quant_table_chroma);
      QuantizedBlock<D> quantized_Cr = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoCr))), quant_table_chroma);

      RunLengthEncodedBlock runlength_Y  = diffEncodeBlock(runLenghtEncodeBlock<D>(traverseBlock<D>(quantized_Y,  traversal_table_luma)),   prev_Y);
      RunLengthEncodedBlock runlength_Cb = diffEncodeBlock(runLenghtEncodeBlock<D>(traverseBlock<D>(quantized_Cb, traversal_table_chroma)), prev_Cb);
      RunLengthEncodedBlock runlength_Cr = diffEncodeBlock(runLenghtEncodeBlock<D>(traverseBlock<D>(quantized_Cr, traversal_table_chroma)), prev_Cr);

      encodeOneBlock(runlength_Y,  huffmap_luma_DC, huffmap_luma_AC, bitstream);
      encodeOneBlock(runlength_Cb, huffmap_chroma_DC, huffmap_chroma_AC, bitstream);
      encodeOneBlock(runlength_Cr, huffmap_chroma_DC, huffmap_chroma_AC, bitstream);
    }
  }

  bitstream.flush();

  return 0;
}

#endif
