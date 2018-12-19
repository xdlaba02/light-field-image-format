/*******************************************************\
* SOUBOR: lfif3d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 12. 2018
\*******************************************************/

#include "endian.h"
#include "compress.h"
#include "lfif_encoder.h"

#include <iostream>
#include <bitset>

vector<size_t> generateZigzagTable(uint64_t size) {
  vector<size_t> table(size * size);

  size_t x = 0;
  size_t y = 0;
  size_t output_index = 0;
  while (true) {
    table[y * size + x] = output_index++;

    if (x < size - 1) {
      x++;
    }
    else if (y < size - 1) {
      y++;
    }
    else {
      break;
    }

    while ((x > 0) && (y < size - 1)) {
      table[y * size + x] = output_index++;
      x--;
      y++;
    }

    table[y * size + x] = output_index++;

    if (y < size - 1) {
      y++;
    }
    else if (x < size - 1) {
      x++;
    }
    else {
      break;
    }

    while ((x < size - 1) && (y > 0)) {
      table[y * size + x] = output_index++;
      x++;
      y--;
    }
  }

  return table;
}

RGBData zigzagShift(const RGBData &rgb_data, uint64_t depth) {
  RGBData zigzag_data(rgb_data.size());

  size_t image_size = rgb_data.size() / depth;

  vector<size_t> zigzag_table = generateZigzagTable(sqrt(depth));

  for (size_t i = 0; i < depth; i++) {
    for (size_t j = 0; j < image_size; j++) {
      zigzag_data[zigzag_table[i] * image_size + j] = rgb_data[i * image_size + j];
    }
  }

  return zigzag_data;
}


int main(int argc, char *argv[]) {
  string input_file_mask  {};
  string output_file_name {};
  uint8_t quality         {};

  if (!parse_args(argc, argv, input_file_mask, output_file_name, quality)) {
    return -1;
  }

  uint64_t width  {};
  uint64_t height {};
  uint64_t depth  {};

  RGBData rgb_data {};

  if (!loadPPMs(input_file_mask, width, height, depth, rgb_data)) {
    return -2;
  }

  rgb_data = zigzagShift(rgb_data, depth);

  /**********************/

  size_t blocks_cnt = ceil(width/8.) * ceil(height/8.) * ceil(depth/8.);

  QuantTable<3> quant_table = scaleQuantTable<3>(baseQuantTable<3>(), quality);
  RefereceBlock<3> reference_block {};

  auto blockize = [&](const YCbCrData &input){
    vector<YCbCrDataBlock<3>> output(blocks_cnt);
    Dimensions<3> dims{width, height, depth};

    auto inputF = [&](size_t index) {
      return input[index];
    };

    auto outputF = [&](size_t block_index, size_t pixel_index) -> YCbCrDataUnit &{
      return output[block_index][pixel_index];
    };

    convertToBlocks<3>(inputF, dims.data(), outputF);

    return output;
  };

  auto quantize = [&](const vector<YCbCrDataBlock<3>> &input) {
    return quantizeBlocks<3>(input, quant_table);
  };

  vector<QuantizedBlock<3>> quantized_Y  = quantize(transformBlocks<3>(blockize(shiftData(convertRGB(rgb_data, RGBtoY)))));
  vector<QuantizedBlock<3>> quantized_Cb = quantize(transformBlocks<3>(blockize(shiftData(convertRGB(rgb_data, RGBtoCb)))));
  vector<QuantizedBlock<3>> quantized_Cr = quantize(transformBlocks<3>(blockize(shiftData(convertRGB(rgb_data, RGBtoCr)))));
  RGBData().swap(rgb_data);

  getReference<3>(quantized_Y,  reference_block);
  getReference<3>(quantized_Cb, reference_block);
  getReference<3>(quantized_Cr, reference_block);

  TraversalTable<3> traversal_table = constructTraversalTableByReference<3>(reference_block);

  RunLengthEncodedImage runlength_Y  = diffEncodePairs(runLenghtEncodeBlocks<3>(traverseBlocks<3>(quantized_Y,  traversal_table)));
  RunLengthEncodedImage runlength_Cb = diffEncodePairs(runLenghtEncodeBlocks<3>(traverseBlocks<3>(quantized_Cb, traversal_table)));
  RunLengthEncodedImage runlength_Cr = diffEncodePairs(runLenghtEncodeBlocks<3>(traverseBlocks<3>(quantized_Cr, traversal_table)));

  vector<QuantizedBlock<3>>().swap(quantized_Y);
  vector<QuantizedBlock<3>>().swap(quantized_Cb);
  vector<QuantizedBlock<3>>().swap(quantized_Cr);

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

  output.write("LFIF-3D\n", 8);

  uint64_t raw_width  = toBigEndian(width);
  uint64_t raw_height = toBigEndian(height);
  uint64_t raw_depth = toBigEndian(depth);

  output.write(reinterpret_cast<char *>(&raw_width),  sizeof(raw_width));
  output.write(reinterpret_cast<char *>(&raw_height), sizeof(raw_height));
  output.write(reinterpret_cast<char *>(&raw_depth), sizeof(raw_depth));

  output.write(reinterpret_cast<const char *>(quant_table.data()), quant_table.size());
  output.write(reinterpret_cast<const char *>(traversal_table.data()), traversal_table.size() * sizeof(traversal_table[0]));

  writeHuffmanTable(codelengths_luma_DC, output);
  writeHuffmanTable(codelengths_luma_AC, output);
  writeHuffmanTable(codelengths_chroma_DC, output);
  writeHuffmanTable(codelengths_chroma_AC, output);

  OBitstream bitstream(output);

  HuffmanMap huffmap_luma_DC   = generateHuffmanMap(codelengths_luma_DC);
  HuffmanMap huffmap_luma_AC   = generateHuffmanMap(codelengths_luma_AC);
  HuffmanMap huffmap_chroma_DC = generateHuffmanMap(codelengths_chroma_DC);
  HuffmanMap huffmap_chroma_AC = generateHuffmanMap(codelengths_chroma_AC);

  for (size_t i = 0; i < blocks_cnt; i++) {
    encodeOnePair(runlength_Y[i][0], huffmap_luma_DC, bitstream);
    for (size_t j = 1; j < runlength_Y[i].size(); j++) {
      encodeOnePair(runlength_Y[i][j], huffmap_luma_AC, bitstream);
    }

    encodeOnePair(runlength_Cb[i][0], huffmap_chroma_DC, bitstream);
    for (size_t j = 1; j < runlength_Cb[i].size(); j++) {
      encodeOnePair(runlength_Cb[i][j], huffmap_chroma_AC, bitstream);
    }

    encodeOnePair(runlength_Cr[i][0], huffmap_chroma_DC, bitstream);
    for (size_t j = 1; j < runlength_Cr[i].size(); j++) {
      encodeOnePair(runlength_Cr[i][j], huffmap_chroma_AC, bitstream);
    }
  }

  bitstream.flush();

  return 0;
}
