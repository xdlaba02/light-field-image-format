/******************************************************************************\
* SOUBOR: lfif_encoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_ENCODER_H
#define LFIF_ENCODER_H

#include "lfif.h"
#include "dct.h"
#include "bitstream.h"

#include <iostream>

void huffmanAddWeightAC(const RunLengthEncodedBlock &input, HuffmanWeights &weights);
void huffmanAddWeightDC(const RunLengthEncodedBlock &input, HuffmanWeights &weights);

HuffmanCodelengths generateHuffmanCodelengths(const HuffmanWeights &weights);
HuffmanMap generateHuffmanMap(const HuffmanCodelengths &codelengths);

void writeHuffmanTable(const HuffmanCodelengths &codelengths, ofstream &stream);

void encodeOneBlock(const RunLengthEncodedBlock &runlength, const HuffmanMap &huffmap_DC, const HuffmanMap &huffmap_AC, OBitstream &stream);
void encodeOnePair(const RunLengthPair &pair, const HuffmanMap &map, OBitstream &stream);

HuffmanClass huffmanClass(RunLengthAmplitudeUnit amplitude);
HuffmanSymbol huffmanSymbol(const RunLengthPair &pair);

void writeMagicNumber(const char *number, ofstream &output);
void writeDimension(uint64_t dim, ofstream &output);

inline YCbCrDataUnit RGBtoY(RGBDataUnit R, RGBDataUnit G, RGBDataUnit B) {
  return (0.299 * R) + (0.587 * G) + (0.114 * B);
}

inline YCbCrDataUnit RGBtoCb(RGBDataUnit R, RGBDataUnit G, RGBDataUnit B) {
  return 128 - (0.168736 * R) - (0.331264 * G) + (0.5 * B);
}

inline YCbCrDataUnit RGBtoCr(RGBDataUnit R, RGBDataUnit G, RGBDataUnit B) {
  return 128 + (0.5 * R) - (0.418688 * G) - (0.081312 * B);
}

template<size_t D>
inline constexpr QuantTable<D> baseQuantTableLuma() {
  constexpr QuantTable<2> base_luma {
    16, 11, 10, 16, 124, 140, 151, 161,
    12, 12, 14, 19, 126, 158, 160, 155,
    14, 13, 16, 24, 140, 157, 169, 156,
    14, 17, 22, 29, 151, 187, 180, 162,
    18, 22, 37, 56, 168, 109, 103, 177,
    24, 35, 55, 64, 181, 104, 113, 192,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 199,
  };

  QuantTable<D> quant_table {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    quant_table[i] = base_luma[i%64];
  }

  return quant_table;
}

template<size_t D>
inline constexpr QuantTable<D> baseQuantTableChroma() {
  constexpr QuantTable<2> base_chroma {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
  };

  QuantTable<D> quant_table {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    quant_table[i] = base_chroma[i%64];
  }

  return quant_table;
}

template<size_t D>
inline QuantTable<D> scaleQuantTable(QuantTable<D> quant_table, const uint8_t quality) {
  float scale_coef = quality < 50 ? (5000.0 / quality) / 100 : (200.0 - 2 * quality) / 100;

  for (size_t i = 0; i < constpow(8, D); i++) {
    quant_table[i] = clamp<float>(quant_table[i] * scale_coef, 1, 255);
  }

  return quant_table;
}

template<size_t D>
struct getBlock {
  template <typename IF, typename OF>
  getBlock(IF &&input, size_t block, const size_t dims[D], OF &&output) {
    size_t blocks_x = 1;
    size_t size_x   = 1;

    for (size_t i = 0; i < D - 1; i++) {
      blocks_x *= ceil(dims[i]/8.0);
      size_x *= dims[i];
    }

    for (size_t pixel = 0; pixel < 8; pixel++) {
      size_t image_y = (block / blocks_x) * 8 + pixel;

      if (image_y >= dims[D-1]) {
        image_y = dims[D-1] - 1;
      }

      auto inputF = [&](size_t image_index){
        return input(image_y * size_x * 3 + image_index);
      };

      auto outputF = [&](size_t pixel_index) -> RGBDataPixel &{
        return output(pixel * constpow(8, D-1) + pixel_index);
      };

      getBlock<D-1>(inputF, block % blocks_x, dims, outputF);
    }
  }
};

template<>
struct getBlock<1> {
  template <typename IF, typename OF>
  getBlock(IF &&input, const size_t block, const size_t dims[1], OF &&output) {
    for (size_t pixel = 0; pixel < 8; pixel++) {
      size_t image = block * 8 + pixel;

      if (image >= dims[0]) {
        image = dims[0] - 1;
      }

      output(pixel).r = input(image * 3 + 0);
      output(pixel).g = input(image * 3 + 1);
      output(pixel).b = input(image * 3 + 2);
    }
  }
};

template<size_t D>
inline YCbCrDataBlock<D> convertRGBDataBlock(const RGBDataBlock<D> &input, function<YCbCrDataUnit(RGBDataUnit, RGBDataUnit, RGBDataUnit)> &&f) {
  YCbCrDataBlock<D> output {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    RGBDataUnit R = input[i].r;
    RGBDataUnit G = input[i].g;
    RGBDataUnit B = input[i].b;

    output[i] = f(R, G, B);
  }

  return output;
}

template<size_t D>
inline YCbCrDataBlock<D> shiftBlock(YCbCrDataBlock<D> input) {
  for (size_t i = 0; i < constpow(8, D); i++) {
    input[i] -= 128;
  }
  return input;
}

template<size_t D>
inline TransformedBlock<D> transformBlock(const YCbCrDataBlock<D> &input) {
  TransformedBlock<D> output {};

  fdct<D>([&](size_t index) -> DCTDataUnit { return input[index];}, [&](size_t index) -> DCTDataUnit & { return output[index]; });

  return output;
}

template<size_t D>
inline QuantizedBlock<D> quantizeBlock(const TransformedBlock<D> &input, const QuantTable<D> &quant_table) {
  QuantizedBlock<D> output;

  for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
    output[pixel_index] = input[pixel_index] / quant_table[pixel_index];
  }

  return output;
}

template<size_t D>
void addToReference(const QuantizedBlock<D>& block, RefereceBlock<D>& ref) {
  for (size_t i = 0; i < constpow(8, D); i++) {
    ref[i] += abs(block[i]);
  }
}

template<size_t D>
inline TraversalTable<D> constructTraversalTableByReference(const RefereceBlock<D> &ref_block) {
  TraversalTable<D> traversal_table    {};
  Block<pair<double, size_t>, D> srt {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    srt[i].first += abs(ref_block[i]);
    srt[i].second = i;
  }

  stable_sort(srt.rbegin(), srt.rend());

  for (size_t i = 0; i < constpow(8, D); i++) {
    traversal_table[srt[i].second] = i;
  }

  return traversal_table;
}

template<size_t D>
TraversalTable<D> constructTraversalTableByRadius() {
  TraversalTable<D>                  traversal_table {};
  Block<pair<double, size_t>, D> srt          {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    for (size_t j = 1; j <= D; j++) {
      size_t coord = (i % constpow(8, j)) / constpow(8, j-1);
      srt[i].first += coord * coord;
    }
    srt[i].second = i;
  }

  stable_sort(srt.begin(), srt.end());

  for (size_t i = 0; i < constpow(8, D); i++) {
    traversal_table[srt[i].second] = i;
  }

  return traversal_table;
}

template<size_t D>
inline TraversalTable<D> constructTraversalTableByDiagonals() {
  TraversalTable<D>                     traversal_table {};
  Block<pair<double, size_t>, D> srt          {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    for (size_t j = 1; j <= D; j++) {
      srt[i].first += (i % constpow(8, j)) / constpow(8, j-1);
    }
    srt[i].second = i;
  }

  stable_sort(srt.begin(), srt.end());

  for (size_t i = 0; i < constpow(8, D); i++) {
    traversal_table[srt[i].second] = i;
  }

  return traversal_table;
}

template<size_t D>
inline TraversedBlock<D> traverseBlock(const QuantizedBlock<D> &input, const TraversalTable<D> &traversal_table) {
  TraversedBlock<D> output {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    output[traversal_table[i]] = input[i];
  }

  return output;
}

template<size_t D>
inline RunLengthEncodedBlock runLenghtEncodeBlock(const TraversedBlock<D> &input) {
  RunLengthEncodedBlock output {};

  output.push_back({0, static_cast<RunLengthAmplitudeUnit>(input[0])});

  size_t zeroes = 0;
  for (size_t pixel_index = 1; pixel_index < constpow(8, D); pixel_index++) {
    if (input[pixel_index] == 0) {
      zeroes++;
    }
    else {
      while (zeroes >= 16) {
        output.push_back({15, 0});
        zeroes -= 16;
      }
      output.push_back({static_cast<RunLengthZeroesCountUnit>(zeroes), input[pixel_index]});
      zeroes = 0;
    }
  }

  output.push_back({0, 0});

  return output;
}

template<size_t D>
void writeQuantTable(const QuantTable<D> &table, ofstream &output) {
  output.write(reinterpret_cast<const char *>(table.data()), table.size() * sizeof(QuantTable<0>::value_type));
}

template<size_t D>
void writeTraversalTable(const TraversalTable<D> &table, ofstream &output) {
  output.write(reinterpret_cast<const char *>(table.data()), table.size() * sizeof(TraversalTable<0>::value_type));
}

template<size_t D>
int LFIFCompress(const RGBData &rgb_data, const uint64_t img_dims[D], uint64_t imgs_cnt, uint8_t quality, const char *output_file_name) {
  size_t blocks_cnt {};
  size_t pixels_cnt {};

  QuantTable<D> quant_table_luma   {};
  QuantTable<D> quant_table_chroma {};

  RefereceBlock<D> reference_block_luma   {};
  RefereceBlock<D> reference_block_chroma {};

  TraversalTable<D> traversal_table_luma   {};
  TraversalTable<D> traversal_table_chroma {};

  HuffmanWeights weights_luma_AC   {};
  HuffmanWeights weights_luma_DC   {};
  HuffmanWeights weights_chroma_AC {};
  HuffmanWeights weights_chroma_DC {};

  HuffmanCodelengths codelengths_luma_DC   {};
  HuffmanCodelengths codelengths_luma_AC   {};
  HuffmanCodelengths codelengths_chroma_DC {};
  HuffmanCodelengths codelengths_chroma_AC {};

  RunLengthAmplitudeUnit prev_Y  {};
  RunLengthAmplitudeUnit prev_Cb {};
  RunLengthAmplitudeUnit prev_Cr {};

  HuffmanMap huffmap_luma_DC   {};
  HuffmanMap huffmap_luma_AC   {};
  HuffmanMap huffmap_chroma_DC {};
  HuffmanMap huffmap_chroma_AC {};

  blocks_cnt = 1;
  pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    blocks_cnt *= ceil(img_dims[i]/8.);
    pixels_cnt *= img_dims[i];
  }

  auto blockAt = [&](size_t img, size_t block) {
    RGBDataBlock<D> output {};

    auto inputF = [&](size_t index) {
      return rgb_data[img * pixels_cnt * 3 + index];
    };

    auto outputF = [&](size_t index) -> RGBDataPixel &{
      return output[index];
    };

    getBlock<D>(inputF, block, img_dims, outputF);

    return output;
  };

  auto diffEncodeBlock = [&](RunLengthEncodedBlock input, RunLengthAmplitudeUnit &prev) {
    RunLengthAmplitudeUnit curr = input[0].amplitude;
    input[0].amplitude -= prev;
    prev = curr;

    return input;
  };

  quant_table_luma   = scaleQuantTable<D>(baseQuantTableLuma<D>(), quality);
  quant_table_chroma = scaleQuantTable<D>(baseQuantTableChroma<D>(), quality);

  for (size_t img = 0; img < imgs_cnt; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      RGBDataBlock<D> rgb_block = blockAt(img, block);

      QuantizedBlock<D> quantized_Y  = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoY))),  quant_table_luma);
      QuantizedBlock<D> quantized_Cb = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoCb))), quant_table_chroma);
      QuantizedBlock<D> quantized_Cr = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoCr))), quant_table_chroma);

      addToReference<D>(quantized_Y,  reference_block_luma);
      addToReference<D>(quantized_Cb, reference_block_chroma);
      addToReference<D>(quantized_Cr, reference_block_chroma);
    }
  }

  traversal_table_luma   = constructTraversalTableByReference<D>(reference_block_luma);
  traversal_table_chroma = constructTraversalTableByReference<D>(reference_block_chroma);

  prev_Y  = 0;
  prev_Cb = 0;
  prev_Cr = 0;

  for (size_t img = 0; img < imgs_cnt; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      RGBDataBlock<D> rgb_block = blockAt(img, block);

      QuantizedBlock<D> quantized_Y  = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoY))),  quant_table_luma);
      QuantizedBlock<D> quantized_Cb = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoCb))), quant_table_chroma);
      QuantizedBlock<D> quantized_Cr = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoCr))), quant_table_chroma);

      RunLengthEncodedBlock runlength_Y  = diffEncodeBlock(runLenghtEncodeBlock<D>(traverseBlock<D>(quantized_Y,  traversal_table_luma)),   prev_Y);
      RunLengthEncodedBlock runlength_Cb = diffEncodeBlock(runLenghtEncodeBlock<D>(traverseBlock<D>(quantized_Cb, traversal_table_chroma)), prev_Cb);
      RunLengthEncodedBlock runlength_Cr = diffEncodeBlock(runLenghtEncodeBlock<D>(traverseBlock<D>(quantized_Cr, traversal_table_chroma)), prev_Cr);

      huffmanAddWeightAC(runlength_Y,  weights_luma_AC);
      huffmanAddWeightDC(runlength_Y,  weights_luma_DC);

      huffmanAddWeightAC(runlength_Cb, weights_chroma_AC);
      huffmanAddWeightAC(runlength_Cr, weights_chroma_AC);
      huffmanAddWeightDC(runlength_Cb, weights_chroma_DC);
      huffmanAddWeightDC(runlength_Cr, weights_chroma_DC);
    }
  }

  codelengths_luma_DC   = generateHuffmanCodelengths(weights_luma_DC);
  codelengths_luma_AC   = generateHuffmanCodelengths(weights_luma_AC);
  codelengths_chroma_DC = generateHuffmanCodelengths(weights_chroma_DC);
  codelengths_chroma_AC = generateHuffmanCodelengths(weights_chroma_AC);

  size_t last_slash_pos = string(output_file_name).find_last_of('/');
  string command("mkdir -p " + string(output_file_name).substr(0, last_slash_pos));
  system(command.c_str());

  ofstream output(output_file_name);
  if (output.fail()) {
    return -1;
  }

  char magic_number[9] = "LFIF-#D\n";
  magic_number[5] = D + '0';

  writeMagicNumber(magic_number, output);

  for (size_t i = 0; i < D; i++) {
    writeDimension(img_dims[i], output);
  }

  writeDimension(imgs_cnt, output);

  writeQuantTable<D>(quant_table_luma, output);
  writeQuantTable<D>(quant_table_chroma, output);

  writeTraversalTable<D>(traversal_table_luma, output);
  writeTraversalTable<D>(traversal_table_chroma, output);

  writeHuffmanTable(codelengths_luma_DC, output);
  writeHuffmanTable(codelengths_luma_AC, output);
  writeHuffmanTable(codelengths_chroma_DC, output);
  writeHuffmanTable(codelengths_chroma_AC, output);

  huffmap_luma_DC   = generateHuffmanMap(codelengths_luma_DC);
  huffmap_luma_AC   = generateHuffmanMap(codelengths_luma_AC);
  huffmap_chroma_DC = generateHuffmanMap(codelengths_chroma_DC);
  huffmap_chroma_AC = generateHuffmanMap(codelengths_chroma_AC);

  prev_Y  = 0;
  prev_Cb = 0;
  prev_Cr = 0;

  OBitstream bitstream(output);

  for (size_t img = 0; img < imgs_cnt; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      RGBDataBlock<D> rgb_block = blockAt(img, block);

      QuantizedBlock<D> quantized_Y  = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoY))), quant_table_luma);
      QuantizedBlock<D> quantized_Cb = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoCb))), quant_table_chroma);
      QuantizedBlock<D> quantized_Cr = quantizeBlock<D>(transformBlock<D>(shiftBlock<D>(convertRGBDataBlock<D>(rgb_block, RGBtoCr))), quant_table_chroma);

      RunLengthEncodedBlock runlength_Y  = diffEncodeBlock(runLenghtEncodeBlock<D>(traverseBlock<D>(quantized_Y,  traversal_table_luma)),   prev_Y);
      RunLengthEncodedBlock runlength_Cb = diffEncodeBlock(runLenghtEncodeBlock<D>(traverseBlock<D>(quantized_Cb, traversal_table_chroma)), prev_Cb);
      RunLengthEncodedBlock runlength_Cr = diffEncodeBlock(runLenghtEncodeBlock<D>(traverseBlock<D>(quantized_Cr, traversal_table_chroma)), prev_Cr);

      encodeOneBlock(runlength_Y,  huffmap_luma_DC, huffmap_luma_AC, bitstream);
      encodeOneBlock(runlength_Cb, huffmap_chroma_DC, huffmap_chroma_AC, bitstream);
      encodeOneBlock(runlength_Cr, huffmap_chroma_DC, huffmap_chroma_AC, bitstream);
    }
  }

  bitstream.flush();

  return 0;
}

#endif
