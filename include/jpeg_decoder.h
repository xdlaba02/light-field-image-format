/*******************************************************\
* SOUBOR: jpeg_decoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#ifndef JPEG_DECODER_H
#define JPEG_DECODER_H

#include "jpeg.h"
#include "dct.h"
#include "bitstream.h"

#include <cassert>
#include <iostream>
#include <iomanip>

using namespace std;

void readHuffmanTable(vector<uint8_t> &counts, vector<uint8_t> &symbols, ifstream &stream);

vector<vector<RunLengthPair>> decodePairs(const vector<uint8_t> &huff_counts_DC, const vector<uint8_t> &huff_counts_AC, const vector<uint8_t> & huff_symbols_DC, const vector<uint8_t> &huff_symbols_AC, const uint64_t count, IBitstream &bitstream);

RunLengthPair decodeOnePair(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream);
uint8_t decodeOneHuffmanSymbol(const vector<uint8_t> &counts, const vector<uint8_t> &symbols, IBitstream &stream);
int16_t decodeOneAmplitude(uint8_t length, IBitstream &stream);

vector<uint8_t> YCbCrToRGB(const vector<uint8_t> &Y_data, const vector<uint8_t> &Cb_data, const vector<uint8_t> &Cr_data);

template<uint8_t D>
inline vector<Block<int16_t, D>> runLenghtDiffDecodePairs(const vector<vector<RunLengthPair>> &pairvecs) {
  vector<Block<int16_t, D>> blocks(pairvecs.size());

  int16_t prev_DC = 0;

  for (uint64_t block_index = 0; block_index < pairvecs.size(); block_index++) {
    const vector<RunLengthPair> &vec   = pairvecs[block_index];
    Block<int16_t, D>           &block = blocks[block_index];

    block[0] = vec[0].amplitude + prev_DC;
    prev_DC = block[0];

    uint16_t pixel_index = 1;
    for (uint64_t i = 1; i < vec.size() - 1; i++) {
      pixel_index += vec[i].zeroes;
      block[pixel_index] = vec[i].amplitude;
      pixel_index++;
    }
  }

  return blocks;
}

template<uint8_t D>
inline vector<Block<int16_t, D>> dezigzagBlocks(const vector<Block<int16_t, D>> &blocks, const ZigzagTable<D> &zigzag_table) {
  vector<Block<int16_t, D>> blocks_dezigzaged(blocks.size());

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    const Block<int16_t, D> &block            = blocks[block_index];
    Block<int16_t, D>       &block_dezigzaged = blocks_dezigzaged[block_index];

    for (uint16_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_dezigzaged[pixel_index] = block[zigzag_table[pixel_index]];
    }
  }

  return blocks_dezigzaged;
}

template<uint8_t D>
inline vector<Block<int32_t, D>> dequantizeBlocks(const vector<Block<int16_t, D>> &blocks, const QuantTable<D> &quant_table) {
  vector<Block<int32_t, D>> blocks_dequantized(blocks.size());

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    const Block<int16_t, D> &block             = blocks[block_index];
    Block<int32_t, D>       &block_dequantized = blocks_dequantized[block_index];

    for (uint64_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_dequantized[pixel_index] = block[pixel_index] * quant_table[pixel_index];
    }
  }

  return blocks_dequantized;
}

template<uint8_t D>
inline vector<Block<float, D>> detransformBlocks(const vector<Block<int32_t, D>> &blocks) {
  vector<Block<float, D>> blocks_detransformed(blocks.size());

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    idct<D>([&](uint64_t index) -> float { return blocks[block_index][index]; }, [&](uint64_t index) -> float & { return blocks_detransformed[block_index][index]; });
  }

  return blocks_detransformed;
}

template<uint8_t D>
inline vector<Block<uint8_t, D>> deshiftBlocks(const vector<Block<float, D>> &blocks) {
  vector<Block<uint8_t, D>> blocks_deshifted(blocks.size());

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    const Block<float, D> &block           = blocks[block_index];
    Block<uint8_t, D>     &block_deshifted = blocks_deshifted[block_index];

    for (uint64_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_deshifted[pixel_index] = clamp(block[pixel_index] + 128., 0., 255.);
    }
  }

  return blocks_deshifted;
}

template<uint8_t D>
inline vector<uint8_t> convertFromBlocks(const vector<Block<uint8_t, D>> &blocks, const array<uint64_t, D> &dims) {
  array<uint64_t, D> block_dims {};
  uint64_t blocks_cnt = 1;
  uint64_t pixels_cnt = 1;

  for (uint8_t i = 0; i < D; i++) {
    block_dims[i] = ceil(dims[i]/8.0);
    blocks_cnt *= block_dims[i];
    pixels_cnt *= dims[i];
  }

  vector<uint8_t> data(pixels_cnt);

  for (uint64_t block_index = 0; block_index < blocks_cnt; block_index++) {
    const Block<uint8_t, D> &block = blocks[block_index];

    array<uint64_t, D> block_coords {};

    uint64_t acc1 = 1;
    uint64_t acc2 = 1;

    for (uint8_t i = 0; i < D; i++) {
      acc1 *= block_dims[i];
      block_coords[i] = (block_index % acc1) / acc2;
      acc2 = acc1;
    }

    for (uint64_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      array<uint8_t, D> pixel_coords {};

      for (uint8_t i = 0; i < D; i++) {
        pixel_coords[i] = (pixel_index % constpow(8, i+1)) / constpow(8, i);
      }

      array<uint64_t, D> image_coords {};

      for (uint8_t i = 0; i < D; i++) {
        image_coords[i] = block_coords[i] * 8 + pixel_coords[i];
      }

      bool ok = true;

      for (uint8_t i = 0; i < D; i++) {
        if (image_coords[i] > dims[i] - 1) {
          ok = false;
          break;
        }
      }

      if (!ok) {
        continue;
      }

      uint64_t real_pixel_index = 0;
      uint64_t dim_acc = 1;

      for (uint8_t i = 0; i < D; i++) {
        real_pixel_index += image_coords[i] * dim_acc;
        dim_acc *= dims[i];
      }

      data[real_pixel_index] = block[pixel_index];
    }
  }

  return data;
}

template<uint8_t D>
inline bool JPEGtoRGB(const char *input_filename, vector<uint64_t> &src_dimensions, vector<uint8_t> &rgb_data) {
  assert(src_dimensions.size() >= D);

  clock_t clock_start {};
  cerr << fixed << setprecision(3);

  cerr << "OPENING INPUT FILE" << endl;
  clock_start = clock();

  ifstream input(input_filename);
  if (input.fail()) {
    return false;
  }

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "READING MAGIC NUMBER" << endl;
  clock_start = clock();

  char cmp_magic_number[9] {"JPEG-XD\n"};
  cmp_magic_number[5] = '0' + D;

  char magic_number[9] {};
  input.read(magic_number, 8);

  string orig_magic_number {cmp_magic_number};
  string file_magic_number {magic_number};

  if (orig_magic_number != file_magic_number) {
    return false;
  }

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "READING IMAGE DIMENSIONS" << endl;
  clock_start = clock();

  for (uint64_t &dim: src_dimensions) {
    uint64_t raw_dim {};
    input.read(reinterpret_cast<char *>(&raw_dim), sizeof(uint64_t));
    dim = fromBigEndian(raw_dim);
  }

  array<uint64_t, D> dimensions {};

  for (uint64_t i = 0; i < src_dimensions.size(); i++) {
    if (i >= D) {
      dimensions[D-1] *= src_dimensions[i];
    }
    else {
      dimensions[i] = src_dimensions[i];
    }
  }

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "READING QUANTIZATION TABLES" << endl;
  clock_start = clock();

  QuantTable<D> quant_table {};
  input.read(reinterpret_cast<char *>(quant_table.data()), quant_table.size());

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "CONSTRUCTING ZIGZAG TABLE" << endl;
  clock_start = clock();

  ZigzagTable<D> zigzag_table = constructZigzagTable<D>(quant_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "READING HUFFMAN TABLES" << endl;
  clock_start = clock();

  vector<uint8_t> huff_counts_luma_DC   {};
  vector<uint8_t> huff_counts_luma_AC   {};
  vector<uint8_t> huff_counts_chroma_DC {};
  vector<uint8_t> huff_counts_chroma_AC {};

  vector<uint8_t> huff_symbols_luma_DC   {};
  vector<uint8_t> huff_symbols_luma_AC   {};
  vector<uint8_t> huff_symbols_chroma_DC {};
  vector<uint8_t> huff_symbols_chroma_AC {};

  readHuffmanTable(huff_counts_luma_DC, huff_symbols_luma_DC, input);
  readHuffmanTable(huff_counts_luma_AC, huff_symbols_luma_AC, input);
  readHuffmanTable(huff_counts_chroma_DC, huff_symbols_chroma_DC, input);
  readHuffmanTable(huff_counts_chroma_AC, huff_symbols_chroma_AC, input);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "READING AND HUFFMAN DECODING BLOCKS" << endl;
  clock_start = clock();

  array<uint64_t, D> block_dimensions {};
  uint64_t blocks_cnt = 1;

  for (uint64_t i = 0; i < D; i++) {
    block_dimensions[i] = ceil(dimensions[i]/8.);
    blocks_cnt *= block_dimensions[i];
  }

  IBitstream bitstream(input);

  vector<vector<RunLengthPair>> runlenght_Y  = decodePairs(huff_counts_luma_DC,   huff_counts_luma_AC,   huff_symbols_luma_DC,   huff_symbols_luma_AC,   blocks_cnt, bitstream);
  vector<vector<RunLengthPair>> runlenght_Cb = decodePairs(huff_counts_chroma_DC, huff_counts_chroma_AC, huff_symbols_chroma_DC, huff_symbols_chroma_AC, blocks_cnt, bitstream);
  vector<vector<RunLengthPair>> runlenght_Cr = decodePairs(huff_counts_chroma_DC, huff_counts_chroma_AC, huff_symbols_chroma_DC, huff_symbols_chroma_AC, blocks_cnt, bitstream);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "RUNLENGTH AND DIFF DECODING" << endl;
  clock_start = clock();

  vector<Block<int16_t, D>> blocks_Y_zigzag  = runLenghtDiffDecodePairs<D>(runlenght_Y);
  vector<Block<int16_t, D>> blocks_Cb_zigzag = runLenghtDiffDecodePairs<D>(runlenght_Cb);
  vector<Block<int16_t, D>> blocks_Cr_zigzag = runLenghtDiffDecodePairs<D>(runlenght_Cr);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "DEZIGZAGING" << endl;
  clock_start = clock();

  vector<Block<int16_t, D>> blocks_Y_quantized  = dezigzagBlocks<D>(blocks_Y_zigzag,  zigzag_table);
  vector<Block<int16_t, D>> blocks_Cb_quantized = dezigzagBlocks<D>(blocks_Cb_zigzag, zigzag_table);
  vector<Block<int16_t, D>> blocks_Cr_quantized = dezigzagBlocks<D>(blocks_Cr_zigzag, zigzag_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "DEQUANTIZING" << endl;
  clock_start = clock();

  vector<Block<int32_t, D>> blocks_Y_transformed  = dequantizeBlocks<D>(blocks_Y_quantized,  quant_table);
  vector<Block<int32_t, D>> blocks_Cb_transformed = dequantizeBlocks<D>(blocks_Cb_quantized, quant_table);
  vector<Block<int32_t, D>> blocks_Cr_transformed = dequantizeBlocks<D>(blocks_Cr_quantized, quant_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "INVERSE DISCRETE COSINE TRANSFORMING" << endl;
  clock_start = clock();

  vector<Block<float, D>> blocks_Y_shifted  = detransformBlocks<D>(blocks_Y_transformed);
  cerr << "#";
  vector<Block<float, D>> blocks_Cb_shifted = detransformBlocks<D>(blocks_Cb_transformed);
  cerr << "#";
  vector<Block<float, D>> blocks_Cr_shifted = detransformBlocks<D>(blocks_Cr_transformed);
  cerr << "# ";

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "DESHIFTING VALUES TO <0, 255>" << endl;
  clock_start = clock();

  vector<Block<uint8_t, D>> blocks_Y  = deshiftBlocks<D>(blocks_Y_shifted);
  vector<Block<uint8_t, D>> blocks_Cb = deshiftBlocks<D>(blocks_Cb_shifted);
  vector<Block<uint8_t, D>> blocks_Cr = deshiftBlocks<D>(blocks_Cr_shifted);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "DEBLOCKING" << endl;
  clock_start = clock();

  vector<uint8_t> Y_data  = convertFromBlocks<D>(blocks_Y,  dimensions);
  vector<uint8_t> Cb_data = convertFromBlocks<D>(blocks_Cb, dimensions);
  vector<uint8_t> Cr_data = convertFromBlocks<D>(blocks_Cr, dimensions);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "COVERTING TO RGB" << endl;
  clock_start = clock();

  rgb_data = YCbCrToRGB(Y_data, Cb_data, Cr_data);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  return true;
}

#endif
