#ifndef COMPRESS_H
#define COMPRESS_H

#include "lfif.h"

#include "endian.h"
#include "lfif_encoder.h"

#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>

using namespace std;

void print_usage(const char *argv0);

bool parse_args(int argc, char *argv[], string &input_file_mask, string &output_file_name, uint8_t &quality);

vector<size_t> getMaskIndexes(const string &file_mask);

bool loadPPMs(string input_file_mask, uint64_t &width, uint64_t &height, uint64_t &image_count, vector<uint8_t> &rgb_data);

void writeMagicNumber(const char *number, ofstream &output);

void writeDimension(uint64_t dim, ofstream &output);

RGBData zigzagShift(const RGBData &rgb_data, uint64_t depth);

template<size_t D>
void writeQuantTable(const QuantTable<D> &table, ofstream &output) {
  output.write(reinterpret_cast<const char *>(table.data()), table.size() * sizeof(QuantTable<0>::value_type));
}

template<size_t D>
void writeTraversalTable(const TraversalTable<D> &table, ofstream &output) {
  output.write(reinterpret_cast<const char *>(table.data()), table.size() * sizeof(TraversalTable<0>::value_type));
}

template<size_t D>
bool compress(const string &output_file_name, uint64_t width, uint64_t height, uint64_t depth, uint8_t quality, RGBData &rgb_data) {
  size_t blocks_cnt = ceil(width/8.) * ceil(height/8.);

  if (D == 2) {
    blocks_cnt *= depth;
  }
  else if (D == 3) {
    blocks_cnt *= ceil(depth/8.);
  }
  else if (D == 4) {
    blocks_cnt *= ceil(sqrt(depth)/8.) * ceil(sqrt(depth)/8.);
  }

  QuantTable<D> quant_table = scaleQuantTable<D>(baseQuantTable<D>(), quality);
  RefereceBlock<D> reference_block {};

  auto blockize = [&](const YCbCrData &input) {
    vector<YCbCrDataBlock<D>> output(blocks_cnt);
    Dimensions<D> dims {width, height};

    size_t blocks_in_one_image = 0;
    size_t cnt = 1;

    if (D == 2) {
      blocks_in_one_image = blocks_cnt / depth;
      cnt = depth;
    }
    else if (D == 3) {
      dims[2] = depth;
    }
    else if (D == 4) {
      dims[2] = static_cast<size_t>(sqrt(depth));
      dims[3] = static_cast<size_t>(sqrt(depth));
    }

    for (size_t i = 0; i < cnt; i++) {
      auto inputF = [&](size_t index) {
        return input[(i * width * height) + index];
      };

      auto outputF = [&](size_t block_index, size_t pixel_index) -> YCbCrDataUnit &{
        return output[(i * blocks_in_one_image) + block_index][pixel_index];
      };

      convertToBlocks<D>(inputF, dims.data(), outputF);
    }

    return output;
  };

  vector<QuantizedBlock<D>> quantized_Y  = quantizeBlocks<D>(transformBlocks<D>(blockize(shiftData(convertRGB(rgb_data, RGBtoY)))), quant_table);
  vector<QuantizedBlock<D>> quantized_Cb = quantizeBlocks<D>(transformBlocks<D>(blockize(shiftData(convertRGB(rgb_data, RGBtoCb)))), quant_table);
  vector<QuantizedBlock<D>> quantized_Cr = quantizeBlocks<D>(transformBlocks<D>(blockize(shiftData(convertRGB(rgb_data, RGBtoCr)))), quant_table);
  RGBData().swap(rgb_data);

  getReference<D>(quantized_Y,  reference_block);
  getReference<D>(quantized_Cb, reference_block);
  getReference<D>(quantized_Cr, reference_block);

  TraversalTable<D> traversal_table = constructTraversalTableByReference<D>(reference_block);

  RunLengthEncodedImage runlength_Y  = diffEncodePairs(runLenghtEncodeBlocks<D>(traverseBlocks<D>(quantized_Y,  traversal_table)));
  RunLengthEncodedImage runlength_Cb = diffEncodePairs(runLenghtEncodeBlocks<D>(traverseBlocks<D>(quantized_Cb, traversal_table)));
  RunLengthEncodedImage runlength_Cr = diffEncodePairs(runLenghtEncodeBlocks<D>(traverseBlocks<D>(quantized_Cr, traversal_table)));

  vector<QuantizedBlock<D>>().swap(quantized_Y);
  vector<QuantizedBlock<D>>().swap(quantized_Cb);
  vector<QuantizedBlock<D>>().swap(quantized_Cr);

  HuffmanWeights weights_luma_AC   {};
  HuffmanWeights weights_luma_DC   {};

  huffmanGetWeightsAC(runlength_Y,  weights_luma_AC);
  huffmanGetWeightsDC(runlength_Y,  weights_luma_DC);

  HuffmanWeights weights_chroma_AC {};
  HuffmanWeights weights_chroma_DC {};

  huffmanGetWeightsAC(runlength_Cb, weights_chroma_AC);
  huffmanGetWeightsAC(runlength_Cr, weights_chroma_AC);
  huffmanGetWeightsDC(runlength_Cb, weights_chroma_DC);
  huffmanGetWeightsDC(runlength_Cr, weights_chroma_DC);

  HuffmanCodelengths codelengths_luma_DC   = generateHuffmanCodelengths(weights_luma_DC);
  HuffmanCodelengths codelengths_luma_AC   = generateHuffmanCodelengths(weights_luma_AC);
  HuffmanCodelengths codelengths_chroma_DC = generateHuffmanCodelengths(weights_chroma_DC);
  HuffmanCodelengths codelengths_chroma_AC = generateHuffmanCodelengths(weights_chroma_AC);

  ofstream output(output_file_name);
  if (output.fail()) {
    cerr << "OUTPUT FAIL" << endl;
    return false;
  }

  if (D == 2) {
    writeMagicNumber("LFIF-2D\n", output);
  }
  else if (D == 3) {
    writeMagicNumber("LFIF-3D\n", output);
  }
  else if (D == 4) {
    writeMagicNumber("LFIF-4D\n", output);
  }

  writeDimension(width, output);
  writeDimension(height, output);
  writeDimension(depth, output);

  writeQuantTable<D>(quant_table, output);
  writeTraversalTable<D>(traversal_table, output);

  writeHuffmanTable(codelengths_luma_DC, output);
  writeHuffmanTable(codelengths_luma_AC, output);
  writeHuffmanTable(codelengths_chroma_DC, output);
  writeHuffmanTable(codelengths_chroma_AC, output);

  HuffmanMap huffmap_luma_DC   = generateHuffmanMap(codelengths_luma_DC);
  HuffmanMap huffmap_luma_AC   = generateHuffmanMap(codelengths_luma_AC);
  HuffmanMap huffmap_chroma_DC = generateHuffmanMap(codelengths_chroma_DC);
  HuffmanMap huffmap_chroma_AC = generateHuffmanMap(codelengths_chroma_AC);

  OBitstream bitstream(output);

  for (size_t i = 0; i < blocks_cnt; i++) {
    encodeOneBlock(runlength_Y[i], huffmap_luma_DC, huffmap_luma_AC, bitstream);
    encodeOneBlock(runlength_Cb[i], huffmap_chroma_DC, huffmap_chroma_AC, bitstream);
    encodeOneBlock(runlength_Cr[i], huffmap_chroma_DC, huffmap_chroma_AC, bitstream);
  }

  bitstream.flush();

  return true;
}

#endif
