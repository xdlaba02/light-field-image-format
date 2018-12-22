#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include "lfif.h"
#include "lfif_decoder.h"

#include <endian.h>

#include <string>
#include <fstream>

using namespace std;

void print_usage(const char *argv0);

bool parse_args(int argc, char *argv[], const char *&input_file_name, const char *&output_file_mask);

bool checkMagicNumber(const string &cmp, ifstream &input);

uint64_t readDimension(ifstream &input);

bool savePPMs(const RGBData &rgb_data, uint64_t width, uint64_t height, uint32_t color_image_count, uint64_t image_count, const string &output_file_mask);

RGBData zigzagDeshift(const RGBData &rgb_data, uint64_t image_count);

template<size_t D>
QuantTable<D> readQuantTable(ifstream &input) {
  QuantTable<D> table {};
  input.read(reinterpret_cast<char *>(table.data()), table.size() * sizeof(QuantTable<0>::value_type));
  return table;
}

template<size_t D>
TraversalTable<D> readTraversalTable(ifstream &input) {
  TraversalTable<D> table {};
  input.read(reinterpret_cast<char *>(table.data()), table.size() * sizeof(TraversalTable<0>::value_type));
  return table;
}

template<size_t D>
bool decompress(const char *input_file_name, RGBData &rgb_data, uint64_t &width, uint64_t &height, uint64_t &image_count) {
  ifstream input(input_file_name);
  if (input.fail()) {
    return false;
  }

  if (D == 2) {
    if (!checkMagicNumber("LFIF-2D\n", input)) {
      return false;
    }
  }
  else if (D == 3) {
    if (!checkMagicNumber("LFIF-3D\n", input)) {
      return false;
    }
  }
  else if (D == 4) {
    if (!checkMagicNumber("LFIF-4D\n", input)) {
      return false;
    }
  }

  width  = readDimension(input);
  height = readDimension(input);
  image_count  = readDimension(input);

  QuantTable<D> quant_table = readQuantTable<D>(input);
  TraversalTable<D> traversal_table = readTraversalTable<D>(input);

  HuffmanTable hufftable_luma_DC = readHuffmanTable(input);
  HuffmanTable hufftable_luma_AC = readHuffmanTable(input);
  HuffmanTable hufftable_chroma_DC = readHuffmanTable(input);
  HuffmanTable hufftable_chroma_AC = readHuffmanTable(input);

  size_t blocks_cnt = ceil(width/8.) * ceil(height/8.);

  if (D == 2) {
    blocks_cnt *= image_count;
  }
  else if (D == 3) {
    blocks_cnt *= ceil(image_count/8.);
  }
  else if (D == 4) {
    blocks_cnt *= ceil(sqrt(image_count)/8.) * ceil(sqrt(image_count)/8.);
  }

  RunLengthEncodedImage pairs_Y(blocks_cnt);
  RunLengthEncodedImage pairs_Cb(blocks_cnt);
  RunLengthEncodedImage pairs_Cr(blocks_cnt);

  IBitstream bitstream(input);

  for (size_t i = 0; i < blocks_cnt; i++) {
    decodeOneBlock(pairs_Y[i],  hufftable_luma_DC, hufftable_luma_AC, bitstream);
    decodeOneBlock(pairs_Cb[i], hufftable_chroma_DC, hufftable_chroma_AC, bitstream);
    decodeOneBlock(pairs_Cr[i], hufftable_chroma_DC, hufftable_chroma_AC, bitstream);
  }

  auto deblockize = [&](const vector<YCbCrDataBlock<D>> &input) {
    YCbCrData output(width * height * image_count);
    size_t dims[D] {width, height};

    size_t blocks_in_one_image = 0;
    size_t cnt = 1;

    if (D == 2) {
      blocks_in_one_image = blocks_cnt / image_count;
      cnt = image_count;
    }
    else if (D == 3) {
      dims[2] = image_count;
    }
    else if (D == 4) {
      dims[2] = static_cast<size_t>(sqrt(image_count));
      dims[3] = static_cast<size_t>(sqrt(image_count));
    }

    for (size_t i = 0; i < cnt; i++) {
      auto inputF = [&](size_t block_index, size_t pixel_index){
        return input[(i * blocks_in_one_image) + block_index][pixel_index];
      };

      auto outputF = [&](size_t index) -> YCbCrDataUnit &{
        return output[(i * width * height) + index];
      };

      convertFromBlocks<D>(inputF, dims, outputF);
    }

    return output;
  };

  auto decode = [&](const RunLengthEncodedImage &input) {
    return deshiftData(deblockize(detransformBlocks<D>(dequantizeBlocks<D>(detraverseBlocks<D>(runLenghtDecodePairs<D>(diffDecodePairs(input)), traversal_table), quant_table))));
  };

  rgb_data = YCbCrToRGB(decode(pairs_Y), decode(pairs_Cb), decode(pairs_Cr));

  return true;
}

#endif
