/*******************************************************\
* SOUBOR: lfif2d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 12. 2018
\*******************************************************/

#include "endian.h"
#include "compress.h"
#include "lfif_encoder.h"

#include <iostream>
#include <bitset>

int main(int argc, char *argv[]) {
  string input_file_mask  {};
  string output_file_name {};
  uint8_t quality         {};

  if (!parse_args(argc, argv, input_file_mask, output_file_name, quality)) {
    return -1;
  }

  uint64_t width  {};
  uint64_t height {};
  uint64_t depth {};
  RGBData rgb_data {};

  if (!loadPPMs(input_file_mask, width, height, depth, rgb_data)) {
    return -2;
  }

  /**********************/

  size_t blocks_cnt = ceil(width/8.) * ceil(height/8.);

  QuantTable<2> quant_table = scaleQuantTable<2>(baseQuantTable<2>(), quality);
  RefereceBlock<2> reference_block {};

  auto blockize = [&](const YCbCrData &input){
    vector<YCbCrDataBlock<2>> output(blocks_cnt * depth);
    Dimensions<2> dims{width, height};

    for (size_t i = 0; i < depth; i++) {
      auto inputF = [&](size_t index) {
        return input[(i * width * height) + index];
      };

      auto outputF = [&](size_t block_index, size_t pixel_index) -> YCbCrDataUnit &{
        return output[(i * blocks_cnt) + block_index][pixel_index];
      };

      convertToBlocks<2>(inputF, dims.data(), outputF);
    }

    return output;
  };

  auto quantize = [&](const vector<YCbCrDataBlock<2>> &input) {
    return quantizeBlocks<2>(input, quant_table);
  };

  vector<QuantizedBlock<2>> quantized_Y  = quantize(transformBlocks<2>(blockize(shiftData(convertRGB(rgb_data, RGBtoY)))));
  vector<QuantizedBlock<2>> quantized_Cb = quantize(transformBlocks<2>(blockize(shiftData(convertRGB(rgb_data, RGBtoCb)))));
  vector<QuantizedBlock<2>> quantized_Cr = quantize(transformBlocks<2>(blockize(shiftData(convertRGB(rgb_data, RGBtoCr)))));
  RGBData().swap(rgb_data);

  getReference<2>(quantized_Y,  reference_block);
  getReference<2>(quantized_Cb, reference_block);
  getReference<2>(quantized_Cr, reference_block);

  TraversalTable<2> traversal_table = constructTraversalTableByReference<2>(reference_block);

  RunLengthEncodedImage runlength_Y  = diffEncodePairs(runLenghtEncodeBlocks<2>(traverseBlocks<2>(quantized_Y,  traversal_table)));
  RunLengthEncodedImage runlength_Cb = diffEncodePairs(runLenghtEncodeBlocks<2>(traverseBlocks<2>(quantized_Cb, traversal_table)));
  RunLengthEncodedImage runlength_Cr = diffEncodePairs(runLenghtEncodeBlocks<2>(traverseBlocks<2>(quantized_Cr, traversal_table)));

  vector<QuantizedBlock<2>>().swap(quantized_Y);
  vector<QuantizedBlock<2>>().swap(quantized_Cb);
  vector<QuantizedBlock<2>>().swap(quantized_Cr);

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

  /*******************************/

  ofstream output(output_file_name);
  if (output.fail()) {
    cerr << "OUTPUT FAIL" << endl;
    return -6;
  }

  writeMagicNumber("LFIF-2D\n", output);

  writeDimension(width, output);
  writeDimension(height, output);
  writeDimension(depth, output);

  writeQuantTable<2>(quant_table, output);
  writeTraversalTable<2>(traversal_table, output);

  writeHuffmanTable(codelengths_luma_DC, output);
  writeHuffmanTable(codelengths_luma_AC, output);
  writeHuffmanTable(codelengths_chroma_DC, output);
  writeHuffmanTable(codelengths_chroma_AC, output);

  HuffmanMap huffmap_luma_DC   = generateHuffmanMap(codelengths_luma_DC);
  HuffmanMap huffmap_luma_AC   = generateHuffmanMap(codelengths_luma_AC);
  HuffmanMap huffmap_chroma_DC = generateHuffmanMap(codelengths_chroma_DC);
  HuffmanMap huffmap_chroma_AC = generateHuffmanMap(codelengths_chroma_AC);

  OBitstream bitstream(output);

  for (size_t i = 0; i < blocks_cnt * depth; i++) {
    encodeOneBlock(runlength_Y[i], huffmap_luma_DC, huffmap_luma_AC, bitstream);
    encodeOneBlock(runlength_Cb[i], huffmap_chroma_DC, huffmap_chroma_AC, bitstream);
    encodeOneBlock(runlength_Cr[i], huffmap_chroma_DC, huffmap_chroma_AC, bitstream);
  }

  bitstream.flush();

  return 0;
}
