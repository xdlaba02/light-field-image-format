/*******************************************************\
* SOUBOR: lfif2d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 16. 12. 2018
\*******************************************************/

#include "lfif_decoder.h"
#include "ppm.h"

#include <getopt.h>

#include <iostream>

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

  QuantTable<D> quant_table {};
  input.read(reinterpret_cast<char *>(quant_table.data()), quant_table.size());

  TraversalTable<D> traversal_table {};
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

  IBitstream bitstream(input);

  blocks_cnt =

  for (uint64_t i = 0; i < image_count; i++) {
    vector<float> Y_data  = convertFromBlocks<D>(detransformBlocks<D>(dequantizeBlocks<D>(dezigzagBlocks<D>(runLenghtDiffDecodePairs<D>(decodePairs(huff_counts_luma_DC,   huff_counts_luma_AC,   huff_symbols_luma_DC,   huff_symbols_luma_AC,   blocks_cnt, bitstream)), traversal_table), quant_table)),  {width, height});
    vector<float> Cb_data = convertFromBlocks<D>(detransformBlocks<D>(dequantizeBlocks<D>(dezigzagBlocks<D>(runLenghtDiffDecodePairs<D>(decodePairs(huff_counts_chroma_DC, huff_counts_chroma_AC, huff_symbols_chroma_DC, huff_symbols_chroma_AC, blocks_cnt, bitstream)), traversal_table), quant_table)), {width, height});
    vector<float> Cr_data = convertFromBlocks<D>(detransformBlocks<D>(dequantizeBlocks<D>(dezigzagBlocks<D>(runLenghtDiffDecodePairs<D>(decodePairs(huff_counts_chroma_DC, huff_counts_chroma_AC, huff_symbols_chroma_DC, huff_symbols_chroma_AC, blocks_cnt, bitstream)), traversal_table), quant_table)), {width, height});

    rgb_data = YCbCrToRGB(Y_data, Cb_data, Cr_data);
  }




  vector<vector<uint8_t>> rgb_data {};


  string output_file_name {output_file_mask};

  vector<uint64_t> mask_indexes {};

  for (uint64_t i = 0; output_file_mask[i] != '\0'; i++) {
    if (output_file_mask[i] == '#') {
      mask_indexes.push_back(i);
    }
  }

  return 0;
}
