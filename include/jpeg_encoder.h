/*******************************************************\
* SOUBOR: jpeg_encoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#ifndef JPEG_ENCODER_H
#define JPEG_ENCODER_H

#include "jpeg.h"
#include "dct.h"
#include "bitstream.h"
#include "constmath.h"

#include <map>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>

uint8_t RGBtoY(uint8_t R, uint8_t G, uint8_t B);
uint8_t RGBtoCb(uint8_t R, uint8_t G, uint8_t B);
uint8_t RGBtoCr(uint8_t R, uint8_t G, uint8_t B);

void huffmanGetWeights(const vector<vector<RunLengthPair>> &pairvecs, map<uint8_t, uint64_t> &weights_AC, map<uint8_t, uint64_t> &weights_DC);

vector<pair<uint64_t, uint8_t>> huffmanGetCodelengths(const map<uint8_t, uint64_t> &weights);
map<uint8_t, Codeword> huffmanGenerateCodewords(const vector<pair<uint64_t, uint8_t>> &codelengths);

void writeHuffmanTable(const vector<pair<uint64_t, uint8_t>> &codelengths, ofstream &stream);

void encodePairs(const vector<vector<RunLengthPair>> &pairvecs, const map<uint8_t, Codeword> &huffcodes_AC, const map<uint8_t, Codeword> &huffcodes_DC, OBitstream &bitstream);
void encodeOnePair(const RunLengthPair &pair, const map<uint8_t, Codeword> &table, OBitstream &stream);

uint8_t huffmanClass(int16_t amplitude);
uint8_t huffmanSymbol(const RunLengthPair &pair);

template<uint8_t D>
QuantTable<D> constructQuantTable(const uint8_t quality) {
  QuantTable<D> quant_table {};

  /* WIKI TABLE
  16,11,10,16, 24, 40, 51, 61,
  12,12,14,19, 26, 58, 60, 55,
  14,13,16,24, 40, 57, 69, 56,
  14,17,22,29, 51, 87, 80, 62,
  18,22,37,56, 68,109,103, 77,
  24,35,55,64, 81,104,113, 92,
  49,64,78,87,103,121,120,101,
  72,92,95,98,112,100,103, 99
  */

  float scale_coef = quality < 50 ? (5000.0 / quality) / 100 : (200.0 - 2 * quality) / 100;

  for (uint16_t i = 0; i < quant_table.size(); i++) {
    uint8_t x  =  i % constpow(8, 1);
    uint8_t y  = (i % constpow(8, 2)) / constpow(8, 1);
    uint8_t xi = (i % constpow(8, 3)) / constpow(8, 2);
    uint8_t yi =  i                                     / constpow(8, 3);

    float radius = sqrt(x*x + y*y + xi*xi + yi*yi);

    float quant_value = (radius+1) * max(max(x, y), max(xi, yi));
    quant_value += 10;

    quant_value *= scale_coef;
    quant_table[i] = clamp(quant_value, 1.f, 128.f);
  }

  return quant_table;
}

template <typename FUNC>
vector<uint8_t> convertRGB(const vector<uint8_t> &rgb_data, FUNC &&function) {
  uint64_t pixels_cnt = rgb_data.size()/3;

  vector<uint8_t> data(pixels_cnt);

  for (uint64_t pixel_index = 0; pixel_index < pixels_cnt; pixel_index++) {
    uint8_t R = rgb_data[3*pixel_index + 0];
    uint8_t G = rgb_data[3*pixel_index + 1];
    uint8_t B = rgb_data[3*pixel_index + 2];

    data[pixel_index] = function(R, G, B);
  }

  return data;
}

template<uint8_t D>
vector<Block<uint8_t, D>> convertToBlocks(const vector<uint8_t> &data, const array<uint64_t, D> &dims) {
  array<uint64_t, D> block_dims {};
  uint64_t blocks_cnt = 1;

  for (uint8_t i = 0; i < D; i++) {
    block_dims[i] = ceil(dims[i]/8.0);
    blocks_cnt *= block_dims[i];
  }

  vector<Block<uint8_t, D>> blocks(blocks_cnt);

  for (uint64_t block_index = 0; block_index < blocks_cnt; block_index++) {
    Block<uint8_t, D> &block = blocks[block_index];

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

      /*******************************************************\
      * V pripade, ze velikost obrazku neni nasobek osmi,
        se blok doplni z krajnich pixelu.
      \*******************************************************/
      for (uint8_t i = 0; i < D; i++) {
        if (image_coords[i] > dims[i] - 1) {
          image_coords[i] = dims[i] - 1;
        }
      }

      uint64_t input_pixel_index = 0;
      uint64_t dim_acc = 1;

      for (uint8_t i = 0; i < D; i++) {
        input_pixel_index += image_coords[i] * dim_acc;
        dim_acc *= dims[i];
      }

      block[pixel_index] = data[input_pixel_index];
    }
  }

  return blocks;
}

template<uint8_t D>
vector<Block<int8_t, D>> shiftBlocks(const vector<Block<uint8_t, D>> &blocks) {
  vector<Block<int8_t, D>> blocks_shifted(blocks.size());

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    const Block<uint8_t, D> &block    = blocks[block_index];
    Block<int8_t, D>  &block_shifted  = blocks_shifted[block_index];

    for (uint16_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_shifted[pixel_index]  = block[pixel_index] - 128;
    }
  }

  return blocks_shifted;
}

template<uint8_t D>
vector<Block<float, D>> transformBlocks(const vector<Block<int8_t, D>> &blocks) {
  vector<Block<float, D>> blocks_transformed(blocks.size());

  auto dct = [](const Block<int8_t, D> &block, Block<float, D> &block_transformed){
    fdct<D>([&](uint64_t index) -> float { return block[index];}, [&](uint64_t index) -> float & { return block_transformed[index]; });
  };

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    dct(blocks[block_index], blocks_transformed[block_index]);
  }

  return blocks_transformed;
}

template<uint8_t D>
vector<Block<int16_t, D>> quantizeBlocks(const vector<Block<float, D>> &blocks, const QuantTable<D> quant_table) {
  vector<Block<int16_t, D>> blocks_quantized(blocks.size());

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    const Block<float, D> &block            = blocks[block_index];
    Block<int16_t, D>     &block_quantized  = blocks_quantized[block_index];

    for (uint64_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_quantized[pixel_index]  = block[pixel_index] * constpow(sqrt(0.25), D) / quant_table[pixel_index];
    }
  }

  return blocks_quantized;
}

template<uint8_t D>
vector<Block<int16_t, D>> zigzagBlocks(const vector<Block<int16_t, D>> &blocks, const ZigzagTable<D> zigzag_table) {
  vector<Block<int16_t, D>> blocks_zigzaged(blocks.size());

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    const Block<int16_t, D> &block          = blocks[block_index];
    Block<int16_t, D>       &block_zigzaged = blocks_zigzaged[block_index];

    for (uint64_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_zigzaged[zigzag_table[pixel_index]] = block[pixel_index];
    }
  }

  return blocks_zigzaged;
}

template<uint8_t D>
vector<vector<RunLengthPair>> runLenghtDiffEncodeBlocks(const vector<Block<int16_t, D>> &blocks) {
  vector<vector<RunLengthPair>> runlengths(blocks.size());

  int16_t prev_DC = 0;

  for (uint64_t block_index = 0; block_index < blocks.size(); block_index++) {
    const Block<int16_t, D> &block     = blocks[block_index];
    vector<RunLengthPair>   &runlength = runlengths[block_index];

    runlength.push_back({0, static_cast<int16_t>(block[0] - prev_DC)});
    prev_DC = block[0];

    uint64_t zeroes = 0;
    for (uint64_t pixel_index = 1; pixel_index < constpow(8, D); pixel_index++) {
      if (block[pixel_index] == 0) {
        zeroes++;
      }
      else {
        while (zeroes >= 16) {
          runlength.push_back({15, 0});
          zeroes -= 16;
        }
        runlength.push_back({static_cast<uint8_t>(zeroes), block[pixel_index]});
        zeroes = 0;
      }
    }

    runlength.push_back({0, 0});
  }

  return runlengths;
}


template<uint8_t D>
bool RGBtoJPEG(const char *output_filename, const vector<uint8_t> &rgb_data, const array<uint64_t, 4> &src_dimensions, const array<uint64_t, D> &dimensions, const uint8_t quality) {
  static_assert(D <= 4);

  clock_t clock_start {};
  cerr << fixed << setprecision(3);

  cerr << "CONVERTING TO YCbCr" << endl;
  clock_start = clock();

  vector<uint8_t> Y_data  = convertRGB(rgb_data, RGBtoY);
  vector<uint8_t> Cb_data = convertRGB(rgb_data, RGBtoCb);
  vector<uint8_t> Cr_data = convertRGB(rgb_data, RGBtoCr);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "CONSTRUCTING QUANTIZATION TABLE" << endl;
  clock_start = clock();

  QuantTable<D> quant_table = constructQuantTable<D>(quality);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "CONSTRUCTING ZIGZAG TABLE" << endl;
  clock_start = clock();

  ZigzagTable<D> zigzag_table = constructZigzagTable<D>(quant_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "COVERTING TO BLOCKS" << endl;
  clock_start = clock();

  vector<Block<uint8_t, D>> blocks_Y  = convertToBlocks<D>(Y_data,  dimensions);
  vector<Block<uint8_t, D>> blocks_Cb = convertToBlocks<D>(Cb_data, dimensions);
  vector<Block<uint8_t, D>> blocks_Cr = convertToBlocks<D>(Cr_data, dimensions);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "SHIFTING VALUES TO <-128, 127>" << endl;
  clock_start = clock();

  vector<Block<int8_t, D>> blocks_Y_shifted  = shiftBlocks<D>(blocks_Y);
  vector<Block<int8_t, D>> blocks_Cb_shifted = shiftBlocks<D>(blocks_Cb);
  vector<Block<int8_t, D>> blocks_Cr_shifted = shiftBlocks<D>(blocks_Cr);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "FORWARD DISCRETE COSINE TRANSFORMING" << endl;
  clock_start = clock();

  vector<Block<float, D>> blocks_Y_transformed  = transformBlocks<D>(blocks_Y_shifted);
  vector<Block<float, D>> blocks_Cb_transformed = transformBlocks<D>(blocks_Cb_shifted);
  vector<Block<float, D>> blocks_Cr_transformed = transformBlocks<D>(blocks_Cr_shifted);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "QUANTIZING" << endl;
  clock_start = clock();

  vector<Block<int16_t, D>> blocks_Y_quantized  = quantizeBlocks<D>(blocks_Y_transformed,  quant_table);
  vector<Block<int16_t, D>> blocks_Cb_quantized = quantizeBlocks<D>(blocks_Cb_transformed, quant_table);
  vector<Block<int16_t, D>> blocks_Cr_quantized = quantizeBlocks<D>(blocks_Cr_transformed, quant_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "ZIGZAGING" << endl;
  clock_start = clock();

  vector<Block<int16_t, D>> blocks_Y_zigzag  = zigzagBlocks<D>(blocks_Y_quantized,  zigzag_table);
  vector<Block<int16_t, D>> blocks_Cb_zigzag = zigzagBlocks<D>(blocks_Cb_quantized, zigzag_table);
  vector<Block<int16_t, D>> blocks_Cr_zigzag = zigzagBlocks<D>(blocks_Cr_quantized, zigzag_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "RUNLENGTH AND DIFF ENCODING" << endl;
  clock_start = clock();

  vector<vector<RunLengthPair>> runlenght_Y  = runLenghtDiffEncodeBlocks<D>(blocks_Y_zigzag);
  vector<vector<RunLengthPair>> runlenght_Cb = runLenghtDiffEncodeBlocks<D>(blocks_Cb_zigzag);
  vector<vector<RunLengthPair>> runlenght_Cr = runLenghtDiffEncodeBlocks<D>(blocks_Cr_zigzag);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "COUNTING RUNLENGTH PAIR WEIGHTS" << endl;
  clock_start = clock();

  map<uint8_t, uint64_t> weights_luma_AC   {};
  map<uint8_t, uint64_t> weights_luma_DC   {};

  map<uint8_t, uint64_t> weights_chroma_AC {};
  map<uint8_t, uint64_t> weights_chroma_DC {};

  huffmanGetWeights(runlenght_Y,  weights_luma_AC, weights_luma_DC);
  huffmanGetWeights(runlenght_Cb, weights_chroma_AC, weights_chroma_DC);
  huffmanGetWeights(runlenght_Cr, weights_chroma_AC, weights_chroma_DC);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "GENERATING HUFFMAN CODE LENGTHS" << endl;
  clock_start = clock();

  vector<pair<uint64_t, uint8_t>> codelengths_luma_DC   = huffmanGetCodelengths(weights_luma_DC);
  vector<pair<uint64_t, uint8_t>> codelengths_luma_AC   = huffmanGetCodelengths(weights_luma_AC);
  vector<pair<uint64_t, uint8_t>> codelengths_chroma_DC = huffmanGetCodelengths(weights_chroma_DC);
  vector<pair<uint64_t, uint8_t>> codelengths_chroma_AC = huffmanGetCodelengths(weights_chroma_AC);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "GENERATING HUFFMAN CODEWORDS" << endl;
  clock_start = clock();

  map<uint8_t, Codeword> huffcodes_luma_DC   = huffmanGenerateCodewords(codelengths_luma_DC);
  map<uint8_t, Codeword> huffcodes_luma_AC   = huffmanGenerateCodewords(codelengths_luma_AC);
  map<uint8_t, Codeword> huffcodes_chroma_DC = huffmanGenerateCodewords(codelengths_chroma_DC);
  map<uint8_t, Codeword> huffcodes_chroma_AC = huffmanGenerateCodewords(codelengths_chroma_AC);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "OPENING OUTPUT FILE TO WRITE" << endl;
  clock_start = clock();

  ofstream output(output_filename);
  if (output.fail()) {
    return false;
  }

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "WRITING MAGIC NUMBER" << endl;
  clock_start = clock();

  output.write("JPEG-", 5);
  output << D;
  output.write("D\n", 2);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "WRITING IMAGE DIMENSIONS" << endl;
  clock_start = clock();

  for (uint64_t dim: src_dimensions) {
    uint64_t raw_dim = toBigEndian(dim);
    output.write(reinterpret_cast<char *>(&raw_dim), sizeof(uint64_t));
  }

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "WRITING QUANTIZATION TABLE" << endl;
  clock_start = clock();

  output.write(reinterpret_cast<char *>(quant_table.data()), quant_table.size());

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "WRITING HUFFMAN TABLES" << endl;
  clock_start = clock();

  writeHuffmanTable(codelengths_luma_DC, output);
  writeHuffmanTable(codelengths_luma_AC, output);
  writeHuffmanTable(codelengths_chroma_DC, output);
  writeHuffmanTable(codelengths_chroma_AC, output);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "HUFFMAN ENCODING AND WRITING BLOCKS" << endl;
  clock_start = clock();

  OBitstream bitstream(output);

  encodePairs(runlenght_Y,  huffcodes_luma_AC,   huffcodes_luma_DC,   bitstream);
  encodePairs(runlenght_Cb, huffcodes_chroma_AC, huffcodes_chroma_DC, bitstream);
  encodePairs(runlenght_Cr, huffcodes_chroma_AC, huffcodes_chroma_DC, bitstream);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "FLUSHING OUTPUT" << endl;
  clock_start = clock();

  bitstream.flush();

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  return true;
}

#endif
