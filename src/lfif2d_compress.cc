/*******************************************************\
* SOUBOR: lfif2d_encoder.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 16. 12. 2018
\*******************************************************/

#include "lfif_encoder.h"
#include "ppm.h"

#include <getopt.h>

#include <iostream>
#include <bitset>

using namespace std;

void print_usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <file-mask> -o <file> -q <quality>" << endl;
}

int main(int argc, char *argv[]) {
  char *input_file_mask {nullptr};
  char *output_file_name    {nullptr};
  char *arg_quality         {nullptr};

  /*******************************************************\
  * Argument parsing
  \*******************************************************/
  char opt;
  while ((opt = getopt(argc, argv, "i:o:q:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
          continue;
        }
        break;

      case 'o':
        if (!output_file_name) {
          output_file_name = optarg;
          continue;
        }
        break;

      case 'q':
        if (!arg_quality) {
          arg_quality = optarg;
          continue;
        }
        break;

      default:
        break;
    }

    print_usage(argv[0]);
    return -1;
  }

  if ((!input_file_mask) || (!output_file_name) || (!arg_quality)) {
    print_usage(argv[0]);
    return -1;
  }

  string input_file_name {input_file_mask};
  uint8_t quality        {};

  int tmp_quality = atoi(arg_quality);
  if ((tmp_quality < 1) || (tmp_quality > 100)) {
    print_usage(argv[0]);
    return -2;
  }
  else {
    quality = tmp_quality;
  }

  vector<uint64_t> mask_indexes {};

  for (uint64_t i = 0; input_file_mask[i] != '\0'; i++) {
    if (input_file_mask[i] == '#') {
      mask_indexes.push_back(i);
    }
  }

  uint64_t image_count {};

  uint64_t width  {};
  uint64_t height {};

  vector<vector<uint8_t>> rgb_data {};

  for (uint64_t image = 0; image < pow(10, mask_indexes.size()); image++) {
    stringstream image_number {};
    image_number << setw(mask_indexes.size()) << setfill('0') << to_string(image);

    for (uint64_t index = 0; index < mask_indexes.size(); index++) {
      input_file_name[mask_indexes[index]] = image_number.str()[index];
    }

    ifstream input(input_file_name);
    if (input.fail()) {
      break;
    }

    image_count++;

    uint64_t image_width {};
    uint64_t image_height {};

    rgb_data.emplace_back();
    if (!readPPM(input, image_width, image_height, rgb_data.back())) {
      cerr << "ERROR: BAD PPM" << endl;
      return -3;
    }

    if (width && height) {
      if ((image_width != width) || (image_height != height)) {
        cerr << "ERROR: WIDTHS NOT SAME" << endl;
        return -4;
      }
    }

    width = image_width;
    height = image_height;
  }

  if (static_cast<uint64_t>(sqrt(image_count) * sqrt(image_count)) != image_count) {
    cerr << "ERROR: NOT SQUARE" << endl;
    return -5;
  }

  vector<Block<float, 2>> blocks_Y  {};
  vector<Block<float, 2>> blocks_Cb {};
  vector<Block<float, 2>> blocks_Cr {};

  for (auto &rgb_image: rgb_data) {
    convertToBlocks<2>(shiftData(convertRGB(rgb_image, RGBtoY)),  {width, height}, blocks_Y);
    convertToBlocks<2>(shiftData(convertRGB(rgb_image, RGBtoCb)), {width, height}, blocks_Cb);
    convertToBlocks<2>(shiftData(convertRGB(rgb_image, RGBtoCr)), {width, height}, blocks_Cr);
  }

  QuantTable<2> quant_table = scaleQuantTable<2>(baseQuantTable<2>(), quality);

  vector<Block<int16_t, 2>> blocks_quantized_Y  = quantizeBlocks<2>(transformBlocks<2>(blocks_Y),  quant_table);
  vector<Block<int16_t, 2>> blocks_quantized_Cb = quantizeBlocks<2>(transformBlocks<2>(blocks_Cb), quant_table);
  vector<Block<int16_t, 2>> blocks_quantized_Cr = quantizeBlocks<2>(transformBlocks<2>(blocks_Cr), quant_table);

  map<uint8_t, uint64_t> weights_luma_AC   {};
  map<uint8_t, uint64_t> weights_luma_DC   {};

  map<uint8_t, uint64_t> weights_chroma_AC {};
  map<uint8_t, uint64_t> weights_chroma_DC {};

  Block<double, 2> reference_block {};

  getReference<2>(blocks_quantized_Y,  reference_block);
  getReference<2>(blocks_quantized_Cb, reference_block);
  getReference<2>(blocks_quantized_Cr, reference_block);

  TraversalTable<2> traversal_table = constructTraversalTableByReference<2>(reference_block);

  vector<vector<RunLengthPair>> runlength_Y  = runLenghtEncodeBlocks<2>(traverseBlocks<2>(blocks_quantized_Y,  traversal_table));
  vector<vector<RunLengthPair>> runlength_Cb = runLenghtEncodeBlocks<2>(traverseBlocks<2>(blocks_quantized_Cb, traversal_table));
  vector<vector<RunLengthPair>> runlength_Cr = runLenghtEncodeBlocks<2>(traverseBlocks<2>(blocks_quantized_Cr, traversal_table));

  diffEncodePairs(runlength_Y);
  diffEncodePairs(runlength_Cb);
  diffEncodePairs(runlength_Cr);

  huffmanGetWeightsAC(runlength_Y,  weights_luma_AC);
  huffmanGetWeightsDC(runlength_Y,  weights_luma_DC);

  huffmanGetWeightsAC(runlength_Cb, weights_chroma_AC);
  huffmanGetWeightsAC(runlength_Cr, weights_chroma_AC);
  huffmanGetWeightsDC(runlength_Cb, weights_chroma_DC);
  huffmanGetWeightsDC(runlength_Cr, weights_chroma_DC);

  vector<pair<uint64_t, uint8_t>> codelengths_luma_DC   = huffmanGetCodelengths(weights_luma_DC);
  vector<pair<uint64_t, uint8_t>> codelengths_luma_AC   = huffmanGetCodelengths(weights_luma_AC);

  vector<pair<uint64_t, uint8_t>> codelengths_chroma_DC = huffmanGetCodelengths(weights_chroma_DC);
  vector<pair<uint64_t, uint8_t>> codelengths_chroma_AC = huffmanGetCodelengths(weights_chroma_AC);

  map<uint8_t, Codeword> huffcodes_luma_DC   = huffmanGenerateCodewords(codelengths_luma_DC);
  map<uint8_t, Codeword> huffcodes_luma_AC   = huffmanGenerateCodewords(codelengths_luma_AC);
  map<uint8_t, Codeword> huffcodes_chroma_DC = huffmanGenerateCodewords(codelengths_chroma_DC);
  map<uint8_t, Codeword> huffcodes_chroma_AC = huffmanGenerateCodewords(codelengths_chroma_AC);

  ofstream output(output_file_name);
  if (output.fail()) {
    return -6;
  }

  output.write("LFIF-2D\n", 8);

  uint64_t raw_width  = toBigEndian(width);
  uint64_t raw_height = toBigEndian(height);
  uint64_t raw_count = toBigEndian(image_count);

  output.write(reinterpret_cast<char *>(&raw_width),  sizeof(raw_width));
  output.write(reinterpret_cast<char *>(&raw_height), sizeof(raw_height));
  output.write(reinterpret_cast<char *>(&raw_count), sizeof(raw_count));

  output.write(reinterpret_cast<const char *>(quant_table.data()), quant_table.size());

  output.write(reinterpret_cast<const char *>(traversal_table.data()), traversal_table.size() * sizeof(traversal_table[0]));

  writeHuffmanTable(codelengths_luma_DC, output);
  writeHuffmanTable(codelengths_luma_AC, output);
  writeHuffmanTable(codelengths_chroma_DC, output);
  writeHuffmanTable(codelengths_chroma_AC, output);

  OBitstream bitstream(output);

  uint64_t blocks_cnt = ceil(width/8.) * ceil(height/8.) * image_count;

  for (uint64_t i = 0; i < blocks_cnt; i++) {
    encodeOnePair(runlength_Y[i][0], huffcodes_luma_DC, bitstream);
    for (uint64_t j = 1; j < runlength_Y[i].size(); j++) {
      encodeOnePair(runlength_Y[i][j], huffcodes_luma_AC, bitstream);
    }

    encodeOnePair(runlength_Cb[i][0], huffcodes_chroma_DC, bitstream);
    for (uint64_t j = 1; j < runlength_Cb[i].size(); j++) {
      encodeOnePair(runlength_Cb[i][j], huffcodes_chroma_AC, bitstream);
    }
    
    encodeOnePair(runlength_Cr[i][0], huffcodes_chroma_DC, bitstream);
    for (uint64_t j = 1; j < runlength_Cr[i].size(); j++) {
      encodeOnePair(runlength_Cr[i][j], huffcodes_chroma_AC, bitstream);
    }
  }

  bitstream.flush();

  return 0;
}
