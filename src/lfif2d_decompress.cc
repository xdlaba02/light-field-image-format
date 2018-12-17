/*******************************************************\
* SOUBOR: lfif2d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 16. 12. 2018
\*******************************************************/

#include "lfif_decoder.h"
#include "ppm.h"

#include <getopt.h>

#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

void print_usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <file> -o <file-mask>" << endl;
}

int main(int argc, char *argv[]) {
  char *input_file_name  {nullptr};
  char *output_file_mask {nullptr};

  /*******************************************************\
  * Argument parsing
  \*******************************************************/
  char opt;
  while ((opt = getopt(argc, argv, "i:o:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_name) {
          input_file_name = optarg;
          continue;
        }
        break;

      case 'o':
        if (!output_file_mask) {
          output_file_mask = optarg;
          continue;
        }
        break;

      default:
        break;
    }

    print_usage(argv[0]);
    return -1;
  }

  if ((!input_file_name) || (!output_file_mask)) {
    print_usage(argv[0]);
    return -1;
  }

  ifstream input(input_file_name);
  if (input.fail()) {
    return -2;
  }

  char magic_number[9] {};
  input.read(magic_number, 8);

  if (string(magic_number) != "LFIF-2D\n") {
    return -3;
  }

  uint64_t raw_width  {};
  uint64_t raw_height {};
  uint64_t raw_count  {};

  input.read(reinterpret_cast<char *>(&raw_width), sizeof(uint64_t));
  input.read(reinterpret_cast<char *>(&raw_height), sizeof(uint64_t));
  input.read(reinterpret_cast<char *>(&raw_count), sizeof(uint64_t));

  uint64_t width  = fromBigEndian(raw_width);
  uint64_t height = fromBigEndian(raw_height);
  uint64_t image_count = fromBigEndian(raw_count);

  QuantTable<2> quant_table {};
  input.read(reinterpret_cast<char *>(quant_table.data()), quant_table.size());

  TraversalTable<2> traversal_table {};
  input.read(reinterpret_cast<char *>(traversal_table.data()), traversal_table.size() * sizeof(traversal_table[0]));

  vector<uint8_t> huff_counts_luma_DC   {};
  vector<uint8_t> huff_counts_luma_AC   {};
  vector<uint8_t> huff_counts_chroma_DC {};
  vector<uint8_t> huff_counts_chroma_AC {};

  vector<uint8_t> huff_symbols_luma_DC   {};
  vector<uint8_t> huff_symbols_luma_AC   {};
  vector<uint8_t> huff_symbols_chroma_DC {};
  vector<uint8_t> huff_symbols_chroma_AC {};

  readHuffmanTable(huff_counts_luma_DC,   huff_symbols_luma_DC,   input);
  readHuffmanTable(huff_counts_luma_AC,   huff_symbols_luma_AC,   input);
  readHuffmanTable(huff_counts_chroma_DC, huff_symbols_chroma_DC, input);
  readHuffmanTable(huff_counts_chroma_AC, huff_symbols_chroma_AC, input);

  size_t blocks_cnt = ceil(width/8.) * ceil(height/8.);

  RunLengthEncodedImage pairs_Y(blocks_cnt  * image_count);
  RunLengthEncodedImage pairs_Cb(blocks_cnt * image_count);
  RunLengthEncodedImage pairs_Cr(blocks_cnt * image_count);

  IBitstream bitstream(input);

  for (size_t i = 0; i < blocks_cnt * image_count; i++) {
    RunLengthPair pair;

    pair = decodeOnePair(huff_counts_luma_DC, huff_symbols_luma_DC, bitstream);
    pairs_Y[i].push_back(pair);
    do {
      pair = decodeOnePair(huff_counts_luma_AC, huff_symbols_luma_AC, bitstream);
      pairs_Y[i].push_back(pair);
    } while((pair.zeroes != 0) || (pair.amplitude != 0));

    pair = decodeOnePair(huff_counts_chroma_DC, huff_symbols_chroma_DC, bitstream);
    pairs_Cb[i].push_back(pair);
    do {
      pair = decodeOnePair(huff_counts_chroma_AC, huff_symbols_chroma_AC, bitstream);
      pairs_Cb[i].push_back(pair);
    } while((pair.zeroes != 0) || (pair.amplitude != 0));

    pair = decodeOnePair(huff_counts_chroma_DC, huff_symbols_chroma_DC, bitstream);
    pairs_Cr[i].push_back(pair);
    do {
      pair = decodeOnePair(huff_counts_chroma_AC, huff_symbols_chroma_AC, bitstream);
      pairs_Cr[i].push_back(pair);
    } while((pair.zeroes != 0) || (pair.amplitude != 0));
  }
  
  vector<YCbCrDataBlock<2>> blocks_Y  = detransformBlocks<2>(dequantizeBlocks<2>(dezigzagBlocks<2>(runLenghtDecodePairs<2>(diffDecodePairs(pairs_Y)), traversal_table), quant_table));
  vector<YCbCrDataBlock<2>> blocks_Cb = detransformBlocks<2>(dequantizeBlocks<2>(dezigzagBlocks<2>(runLenghtDecodePairs<2>(diffDecodePairs(pairs_Cb)), traversal_table), quant_table));
  vector<YCbCrDataBlock<2>> blocks_Cr = detransformBlocks<2>(dequantizeBlocks<2>(dezigzagBlocks<2>(runLenghtDecodePairs<2>(diffDecodePairs(pairs_Cr)), traversal_table), quant_table));

  vector<size_t> mask_indexes {};

  for (size_t i = 0; output_file_mask[i] != '\0'; i++) {
    if (output_file_mask[i] == '#') {
      mask_indexes.push_back(i);
    }
  }

  string output_file_name {output_file_mask};

  for (size_t image = 0; image < image_count; image++) {
    YCbCrData Y_data  = deshiftData(convertFromBlocks<2>(blocks_Y.data()  + image * blocks_cnt, {width, height}));
    YCbCrData Cb_data = deshiftData(convertFromBlocks<2>(blocks_Cb.data() + image * blocks_cnt, {width, height}));
    YCbCrData Cr_data = deshiftData(convertFromBlocks<2>(blocks_Cr.data() + image * blocks_cnt, {width, height}));

    RGBData rgb_data = YCbCrToRGB(Y_data, Cb_data, Cr_data);

    stringstream image_number {};
    image_number << setw(mask_indexes.size()) << setfill('0') << to_string(image);

    for (size_t index = 0; index < mask_indexes.size(); index++) {
      output_file_name[mask_indexes[index]] = image_number.str()[index];
    }

    ofstream output(output_file_name);
    if (output.fail()) {
      return -4;
    }

    writePPM(output, width, height, rgb_data.data());
  }

  return 0;
}
