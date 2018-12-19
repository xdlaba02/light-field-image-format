/*******************************************************\
* SOUBOR: lfif3d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 12. 2018
\*******************************************************/

#include "endian.h"
#include "decompress.h"
#include "lfif_decoder.h"
#include "ppm.h"

#include <getopt.h>

#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

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

RGBData zigzagDeshift(const RGBData &rgb_data, uint64_t depth) {
  RGBData zigzag_data(rgb_data.size());

  size_t image_size = rgb_data.size() / depth;

  vector<size_t> zigzag_table = generateZigzagTable(sqrt(depth));

  for (size_t i = 0; i < depth; i++) {
    for (size_t j = 0; j < image_size; j++) {
      zigzag_data[i * image_size + j] = rgb_data[zigzag_table[i] * image_size + j];
    }
  }

  return zigzag_data;
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

  if (string(magic_number) != "LFIF-3D\n") {
    return -3;
  }

  uint64_t raw_width  {};
  uint64_t raw_height {};
  uint64_t raw_depth  {};

  input.read(reinterpret_cast<char *>(&raw_width), sizeof(uint64_t));
  input.read(reinterpret_cast<char *>(&raw_height), sizeof(uint64_t));
  input.read(reinterpret_cast<char *>(&raw_depth), sizeof(uint64_t));

  uint64_t width  = fromBigEndian(raw_width);
  uint64_t height = fromBigEndian(raw_height);
  uint64_t depth = fromBigEndian(raw_depth);

  QuantTable<3> quant_table {};
  input.read(reinterpret_cast<char *>(quant_table.data()), quant_table.size());

  TraversalTable<3> traversal_table {};
  input.read(reinterpret_cast<char *>(traversal_table.data()), traversal_table.size() * sizeof(traversal_table[0]));

  HuffmanTable hufftable_luma_DC = readHuffmanTable(input);
  HuffmanTable hufftable_luma_AC = readHuffmanTable(input);
  HuffmanTable hufftable_chroma_DC = readHuffmanTable(input);
  HuffmanTable hufftable_chroma_AC = readHuffmanTable(input);

  size_t blocks_cnt = ceil(width/8.) * ceil(height/8.) * ceil(depth/8.);

  RunLengthEncodedImage pairs_Y(blocks_cnt);
  RunLengthEncodedImage pairs_Cb(blocks_cnt);
  RunLengthEncodedImage pairs_Cr(blocks_cnt);

  IBitstream bitstream(input);

  for (size_t i = 0; i < blocks_cnt; i++) {
    RunLengthPair pair;

    pair = decodeOnePair(hufftable_luma_DC, bitstream);
    pairs_Y[i].push_back(pair);
    do {
      pair = decodeOnePair(hufftable_luma_AC, bitstream);
      pairs_Y[i].push_back(pair);
    } while((pair.zeroes != 0) || (pair.amplitude != 0));

    pair = decodeOnePair(hufftable_chroma_DC, bitstream);
    pairs_Cb[i].push_back(pair);
    do {
      pair = decodeOnePair(hufftable_chroma_AC, bitstream);
      pairs_Cb[i].push_back(pair);
    } while((pair.zeroes != 0) || (pair.amplitude != 0));

    pair = decodeOnePair(hufftable_chroma_DC, bitstream);
    pairs_Cr[i].push_back(pair);
    do {
      pair = decodeOnePair(hufftable_chroma_AC, bitstream);
      pairs_Cr[i].push_back(pair);
    } while((pair.zeroes != 0) || (pair.amplitude != 0));
  }

  auto deblockize = [&](const vector<YCbCrDataBlock<3>> &input){
    YCbCrData output(width * height * depth);
    Dimensions<3> dims{width, height, depth};

    auto inputF = [&](size_t block_index, size_t pixel_index) {
      return input[block_index][pixel_index];
    };

    auto outputF = [&](size_t index) -> YCbCrDataUnit & {
      return output[index];
    };

    convertFromBlocks<3>(inputF, dims.data(), outputF);

    return output;
  };

  auto decode = [&](const RunLengthEncodedImage &input) {
    return deshiftData(deblockize(detransformBlocks<3>(dequantizeBlocks<3>(detraverseBlocks<3>(runLenghtDecodePairs<3>(diffDecodePairs(input)), traversal_table), quant_table))));
  };

  RGBData rgb_data = YCbCrToRGB(decode(pairs_Y), decode(pairs_Cb), decode(pairs_Cr));

  rgb_data = zigzagDeshift(rgb_data, depth);

  /*******************************************/

  vector<size_t> mask_indexes {};

  for (size_t i = 0; output_file_mask[i] != '\0'; i++) {
    if (output_file_mask[i] == '#') {
      mask_indexes.push_back(i);
    }
  }

  string output_file_name {output_file_mask};

  for (size_t image = 0; image < depth; image++) {
    stringstream image_number {};
    image_number << setw(mask_indexes.size()) << setfill('0') << to_string(image);

    for (size_t index = 0; index < mask_indexes.size(); index++) {
      output_file_name[mask_indexes[index]] = image_number.str()[index];
    }

    ofstream output(output_file_name);
    if (output.fail()) {
      return -4;
    }

    writePPM(output, width, height, rgb_data.data() + image * width * height * 3);
  }

  return 0;
}
