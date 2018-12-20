/*******************************************************\
* SOUBOR: lfif4d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 20. 12. 2018
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

  if (!checkMagicNumber("LFIF-4D\n", input)) {
    return -3;
  }

  uint64_t width  = readDimension(input);
  uint64_t height = readDimension(input);
  uint64_t depth  = readDimension(input);

  QuantTable<4> quant_table = readQuantTable<4>(input);
  TraversalTable<4> traversal_table = readTraversalTable<4>(input);

  HuffmanTable hufftable_luma_DC = readHuffmanTable(input);
  HuffmanTable hufftable_luma_AC = readHuffmanTable(input);
  HuffmanTable hufftable_chroma_DC = readHuffmanTable(input);
  HuffmanTable hufftable_chroma_AC = readHuffmanTable(input);

  size_t blocks_cnt = ceil(width/8.) * ceil(height/8.) * ceil(sqrt(depth)/8.) * ceil(sqrt(depth)/8.);

  RunLengthEncodedImage pairs_Y(blocks_cnt);
  RunLengthEncodedImage pairs_Cb(blocks_cnt);
  RunLengthEncodedImage pairs_Cr(blocks_cnt);

  IBitstream bitstream(input);

  for (size_t i = 0; i < blocks_cnt; i++) {
    decodeOneBlock(pairs_Y[i],  hufftable_luma_DC, hufftable_luma_AC, bitstream);
    decodeOneBlock(pairs_Cb[i], hufftable_chroma_DC, hufftable_chroma_AC, bitstream);
    decodeOneBlock(pairs_Cr[i], hufftable_chroma_DC, hufftable_chroma_AC, bitstream);
  }

  auto deblockize = [&](const vector<YCbCrDataBlock<4>> &input){
    YCbCrData output(width * height * depth);
    Dimensions<4> dims{width, height, sqrt(depth), sqrt(depth)};

    auto inputF = [&](size_t block_index, size_t pixel_index) {
      return input[block_index][pixel_index];
    };

    auto outputF = [&](size_t index) -> YCbCrDataUnit & {
      return output[index];
    };

    convertFromBlocks<4>(inputF, dims.data(), outputF);

    return output;
  };

  auto decode = [&](const RunLengthEncodedImage &input) {
    return deshiftData(deblockize(detransformBlocks<4>(dequantizeBlocks<4>(detraverseBlocks<4>(runLenghtDecodePairs<4>(diffDecodePairs(input)), traversal_table), quant_table))));
  };

  RGBData rgb_data = YCbCrToRGB(decode(pairs_Y), decode(pairs_Cb), decode(pairs_Cr));

  /*******************************************/

  if (!savePPMs(output_file_mask, width, height, depth, rgb_data)) {
    return -4;
  }

  return 0;
}
