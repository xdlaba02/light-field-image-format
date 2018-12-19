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

vector<uint8_t> zigzagDeshift(const RGBData &shifted_data, uint64_t ix, uint64_t iy) {
  vector<uint8_t> rgb_data(shifted_data.size());

  uint64_t size_2d = shifted_data.size() / (ix * iy);

  uint64_t shift_index = 0;
  uint64_t x = 0;
  uint64_t y = 0;
  auto index = [&]() -> unsigned { return y*size_2d*ix + x*size_2d; };

  while (true) {
    for (uint64_t i = 0; i < size_2d; i++) {
      rgb_data[index()+i] = shifted_data[shift_index++];
    }

    if (x < ix - 1) {
      x++;
    }
    else if (y < iy - 1) {
      y++;
    }
    else {
      break;
    }

    while ((x > 0) && (y < iy - 1)) {
      for (uint64_t i = 0; i < size_2d; i++) {
        rgb_data[index()+i] = shifted_data[shift_index++];
      }
      x--;
      y++;
    }

    for (uint64_t i = 0; i < size_2d; i++) {
      rgb_data[index()+i] = shifted_data[shift_index++];
    }

    if (y < iy - 1) {
      y++;
    }
    else if (x < ix - 1) {
      x++;
    }
    else {
      break;
    }

    while ((x < ix - 1) && (y > 0)) {
      for (uint64_t i = 0; i < size_2d; i++) {
        rgb_data[index()+i] = shifted_data[shift_index++];
      }
      x++;
      y--;
    }
  }

  return rgb_data;
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

  HuffmanTable hufftable_luma_DC = readHuffmanTable(input);
  HuffmanTable hufftable_luma_AC = readHuffmanTable(input);
  HuffmanTable hufftable_chroma_DC = readHuffmanTable(input);
  HuffmanTable hufftable_chroma_AC = readHuffmanTable(input);

  size_t blocks_cnt = ceil(width/8.) * ceil(height/8.);

  RunLengthEncodedImage pairs_Y(blocks_cnt  * image_count);
  RunLengthEncodedImage pairs_Cb(blocks_cnt * image_count);
  RunLengthEncodedImage pairs_Cr(blocks_cnt * image_count);

  IBitstream bitstream(input);

  for (size_t i = 0; i < blocks_cnt * image_count; i++) {
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

  auto deblockize = [&](const vector<YCbCrDataBlock<2>> &input) {
    YCbCrData output(width * height * image_count);
    Dimensions<2> dims{width, height};
    for (size_t i = 0; i < image_count; i++) {
      convertFromBlocks<2>([&](size_t block_index, size_t pixel_index){ return input[blocks_cnt * i + block_index][pixel_index]; }, dims.data(), [&](size_t index) -> YCbCrDataUnit &{ return output[i * width * height + index]; });
    }
    return output;
  };

  auto decode = [&](const RunLengthEncodedImage &input) {
    return deshiftData(deblockize(detransformBlocks<2>(dequantizeBlocks<2>(detraverseBlocks<2>(runLenghtDecodePairs<2>(diffDecodePairs(input)), traversal_table), quant_table))));
  };

  RGBData rgb_data = YCbCrToRGB(decode(pairs_Y), decode(pairs_Cb), decode(pairs_Cr));

  cerr << long(*(rgb_data.end() - 2)) << ", " << long(*(rgb_data.end() - 2)) << ", " << long(*(rgb_data.end() - 1)) << endl;

  /*******************************************/

  vector<size_t> mask_indexes {};

  for (size_t i = 0; output_file_mask[i] != '\0'; i++) {
    if (output_file_mask[i] == '#') {
      mask_indexes.push_back(i);
    }
  }

  string output_file_name {output_file_mask};

  for (size_t image = 0; image < image_count; image++) {
    cerr << "writing image " << image << endl;
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
