/*******************************************************\
* SOUBOR: lfif2d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 12. 2018
\*******************************************************/

#include "endian.h"
#include "decompress.h"
#include "lfif_decoder.h"
#include "ppm.h"

#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

int main(int argc, char *argv[]) {
  string input_file_name  {};
  string output_file_mask {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return -1;
  }

  ifstream input(input_file_name);
  if (input.fail()) {
    return -2;
  }

  if (!checkMagicNumber("LFIF-2D\n", input)) {
    return -3;
  }

  uint64_t width  = readDimension(input);
  uint64_t height = readDimension(input);
  uint64_t depth  = readDimension(input);

  QuantTable<2> quant_table = readQuantTable<2>(input);
  TraversalTable<2> traversal_table = readTraversalTable<2>(input);

  HuffmanTable hufftable_luma_DC = readHuffmanTable(input);
  HuffmanTable hufftable_luma_AC = readHuffmanTable(input);
  HuffmanTable hufftable_chroma_DC = readHuffmanTable(input);
  HuffmanTable hufftable_chroma_AC = readHuffmanTable(input);

  size_t blocks_cnt = ceil(width/8.) * ceil(height/8.);

  RunLengthEncodedImage pairs_Y(blocks_cnt  * depth);
  RunLengthEncodedImage pairs_Cb(blocks_cnt * depth);
  RunLengthEncodedImage pairs_Cr(blocks_cnt * depth);

  IBitstream bitstream(input);

  for (size_t i = 0; i < blocks_cnt * depth; i++) {
    decodeOneBlock(pairs_Y[i],  hufftable_luma_DC, hufftable_luma_AC, bitstream);
    decodeOneBlock(pairs_Cb[i], hufftable_chroma_DC, hufftable_chroma_AC, bitstream);
    decodeOneBlock(pairs_Cr[i], hufftable_chroma_DC, hufftable_chroma_AC, bitstream);
  }

  auto deblockize = [&](const vector<YCbCrDataBlock<2>> &input) {
    YCbCrData output(width * height * depth);
    Dimensions<2> dims{width, height};
    for (size_t i = 0; i < depth; i++) {
      convertFromBlocks<2>([&](size_t block_index, size_t pixel_index){ return input[blocks_cnt * i + block_index][pixel_index]; }, dims.data(), [&](size_t index) -> YCbCrDataUnit &{ return output[i * width * height + index]; });
    }
    return output;
  };

  auto decode = [&](const RunLengthEncodedImage &input) {
    return deshiftData(deblockize(detransformBlocks<2>(dequantizeBlocks<2>(detraverseBlocks<2>(runLenghtDecodePairs<2>(diffDecodePairs(input)), traversal_table), quant_table))));
  };

  RGBData rgb_data = YCbCrToRGB(decode(pairs_Y), decode(pairs_Cb), decode(pairs_Cr));

  /*******************************************/

  vector<size_t> mask_indexes {};

  for (size_t i = 0; output_file_mask[i] != '\0'; i++) {
    if (output_file_mask[i] == '#') {
      mask_indexes.push_back(i);
    }
  }

  for (size_t image = 0; image < depth; image++) {
    stringstream image_number {};
    image_number << setw(mask_indexes.size()) << setfill('0') << to_string(image);

    for (size_t index = 0; index < mask_indexes.size(); index++) {
      output_file_mask[mask_indexes[index]] = image_number.str()[index];
    }

    ofstream output(output_file_mask);
    if (output.fail()) {
      return -4;
    }

    writePPM(output, width, height, rgb_data.data() + image * width * height * 3);
  }

  return 0;
}
