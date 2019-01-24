/******************************************************************************\
* SOUBOR: lfif_encoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_ENCODER_H
#define LFIF_ENCODER_H

#include "lfif.h"
#include "dct.h"
#include "bitstream.h"

YCbCrData shiftData(YCbCrData data);

void huffmanGetWeightsAC(const RunLengthEncodedImage &pairvecs, HuffmanWeights &weights);
void huffmanGetWeightsDC(const RunLengthEncodedImage &pairvecs, HuffmanWeights &weights);

RunLengthEncodedImage diffEncodePairs(RunLengthEncodedImage runlengths);

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

template <typename F>
inline YCbCrData convertRGB(const RGBData &rgb_data, F &&function) {
  size_t pixels_cnt = rgb_data.size()/3;

  YCbCrData data(pixels_cnt);

  for (size_t pixel_index = 0; pixel_index < pixels_cnt; pixel_index++) {
    RGBDataUnit R = rgb_data[3*pixel_index + 0];
    RGBDataUnit G = rgb_data[3*pixel_index + 1];
    RGBDataUnit B = rgb_data[3*pixel_index + 2];

    data[pixel_index] = function(R, G, B);
  }

  return data;
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
struct convertToBlocks {
  template <typename IF, typename OF>
  convertToBlocks(IF &&input, const size_t dims[D], OF &&output) {
    size_t blocks_x = 1;
    size_t size_x   = 1;

    for (size_t i = 0; i < D - 1; i++) {
      blocks_x *= ceil(dims[i]/8.0);
      size_x *= dims[i];
    }

    size_t blocks = ceil(dims[D-1]/8.0);

    for (size_t block = 0; block < blocks; block++) {
      for (size_t pixel = 0; pixel < 8; pixel++) {
        size_t image = block * 8 + pixel;

        if (image >= dims[D-1]) {
          image = dims[D-1] - 1;
        }

        auto inputF = [&](size_t image_index){
          return input(image * size_x + image_index);
        };

        auto outputF = [&](size_t block_index, size_t pixel_index) -> YCbCrDataUnit &{
          return output((block * blocks_x) + block_index, (pixel * constpow(8, D-1)) + pixel_index);
        };

        convertToBlocks<D-1>(inputF, dims, outputF);
      }
    }
  }
};

template<>
struct convertToBlocks<1> {
  template <typename IF, typename OF>
  convertToBlocks(IF &&input, const size_t dims[1], OF &&output) {
    size_t blocks = ceil(dims[0]/8.0);

    for (size_t block = 0; block < blocks; block++) {
      for (size_t pixel = 0; pixel < 8; pixel++) {
        size_t image = block * 8 + pixel;

        if (image >= dims[0]) {
          image = dims[0] - 1;
        }

        output(block, pixel) = input(image);
      }
    }
  }
};

template<size_t D>
vector<TransformedBlock<D>> transformBlocks(const vector<YCbCrDataBlock<D>> &blocks) {
  vector<TransformedBlock<D>> blocks_transformed(blocks.size());

  auto dct = [](const YCbCrDataBlock<D> &block, TransformedBlock<D> &block_transformed){
    fdct<D>([&](size_t index) -> DCTDataUnit { return block[index];}, [&](size_t index) -> DCTDataUnit & { return block_transformed[index]; });
  };

  for (size_t block_index = 0; block_index < blocks.size(); block_index++) {
    dct(blocks[block_index], blocks_transformed[block_index]);
  }

  return blocks_transformed;
}

template<size_t D>
inline vector<QuantizedBlock<D>> quantizeBlocks(const vector<TransformedBlock<D>> &blocks, const QuantTable<D> &quant_table) {
  vector<QuantizedBlock<D>> blocks_quantized(blocks.size());

  for (size_t block_index = 0; block_index < blocks.size(); block_index++) {
    const TransformedBlock<D> &block            = blocks[block_index];
    QuantizedBlock<D>         &block_quantized  = blocks_quantized[block_index];

    for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_quantized[pixel_index] = block[pixel_index] / quant_table[pixel_index];
    }
  }

  return blocks_quantized;
}

template<size_t D>
inline void getReference(const vector<QuantizedBlock<D>> &blocks, RefereceBlock<D> &ref_block) {
  for (auto &block: blocks) {
    for (size_t i = 0; i < constpow(8, D); i++) {
      ref_block[i] += abs(block[i]);
    }
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
inline vector<TraversedBlock<D>> traverseBlocks(const vector<QuantizedBlock<D>> &blocks, const TraversalTable<D> &traversal_table) {
  vector<TraversedBlock<D>> blocks_traversed(blocks.size());

  for (size_t block_index = 0; block_index < blocks.size(); block_index++) {
    const QuantizedBlock<D> &block          = blocks[block_index];
    TraversedBlock<D>       &block_traversed = blocks_traversed[block_index];

    for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
      block_traversed[traversal_table[pixel_index]] = block[pixel_index];
    }
  }

  return blocks_traversed;
}

template<size_t D>
inline RunLengthEncodedImage runLenghtEncodeBlocks(const vector<TraversedBlock<D>> &blocks) {
  RunLengthEncodedImage runlengths(blocks.size());

  for (size_t block_index = 0; block_index < blocks.size(); block_index++) {
    const TraversedBlock<D>  &block     = blocks[block_index];
    RunLengthEncodedBlock &runlength = runlengths[block_index];

    runlength.push_back({0, static_cast<RunLengthAmplitudeUnit>(block[0])});

    size_t zeroes = 0;
    for (size_t pixel_index = 1; pixel_index < constpow(8, D); pixel_index++) {
      if (block[pixel_index] == 0) {
        zeroes++;
      }
      else {
        while (zeroes >= 16) {
          runlength.push_back({15, 0});
          zeroes -= 16;
        }
        runlength.push_back({static_cast<RunLengthZeroesCountUnit>(zeroes), block[pixel_index]});
        zeroes = 0;
      }
    }

    runlength.push_back({0, 0});
  }

  return runlengths;
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
int LFIFCompress(RGBData &rgb_data, const uint64_t img_dims[D], uint64_t imgs_cnt, uint8_t quality, const char *output_file_name) {
  size_t blocks_cnt = 1;
  size_t pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    blocks_cnt *= ceil(img_dims[i]/8.);
    pixels_cnt *= img_dims[i];
  }

  auto blockize = [&](const YCbCrData &input) {
    vector<YCbCrDataBlock<D>> output(blocks_cnt * imgs_cnt);

    for (size_t i = 0; i < imgs_cnt; i++) {
      auto inputF = [&](size_t index) {
        return input[i * pixels_cnt + index];
      };

      auto outputF = [&](size_t block_index, size_t pixel_index) -> YCbCrDataUnit &{
        return output[i * blocks_cnt + block_index][pixel_index];
      };

      convertToBlocks<D>(inputF, img_dims, outputF);
    }

    return output;
  };

  QuantTable<D> quant_table_luma = scaleQuantTable<D>(baseQuantTableLuma<D>(), quality);
  QuantTable<D> quant_table_chroma = scaleQuantTable<D>(baseQuantTableChroma<D>(), quality);

  vector<QuantizedBlock<D>> quantized_Y  = quantizeBlocks<D>(transformBlocks<D>(blockize(shiftData(convertRGB(rgb_data, RGBtoY)))), quant_table_luma);
  vector<QuantizedBlock<D>> quantized_Cb = quantizeBlocks<D>(transformBlocks<D>(blockize(shiftData(convertRGB(rgb_data, RGBtoCb)))), quant_table_chroma);
  vector<QuantizedBlock<D>> quantized_Cr = quantizeBlocks<D>(transformBlocks<D>(blockize(shiftData(convertRGB(rgb_data, RGBtoCr)))), quant_table_chroma);

  /********************************************\
  * Free unused RGB Buffer
  \********************************************/
  RGBData().swap(rgb_data);

  /********************************************\
  * Construt traversal table by reference block
  \********************************************/
  RefereceBlock<D> reference_block_luma {};
  RefereceBlock<D> reference_block_chroma {};

  getReference<D>(quantized_Y,  reference_block_luma);

  getReference<D>(quantized_Cb, reference_block_chroma);
  getReference<D>(quantized_Cr, reference_block_chroma);

  TraversalTable<D> traversal_table_luma = constructTraversalTableByReference<D>(reference_block_luma);
  TraversalTable<D> traversal_table_chroma = constructTraversalTableByReference<D>(reference_block_chroma);

  RunLengthEncodedImage runlength_Y  = diffEncodePairs(runLenghtEncodeBlocks<D>(traverseBlocks<D>(quantized_Y,  traversal_table_luma)));
  RunLengthEncodedImage runlength_Cb = diffEncodePairs(runLenghtEncodeBlocks<D>(traverseBlocks<D>(quantized_Cb, traversal_table_chroma)));
  RunLengthEncodedImage runlength_Cr = diffEncodePairs(runLenghtEncodeBlocks<D>(traverseBlocks<D>(quantized_Cr, traversal_table_chroma)));

  /********************************************\
  * Free unused vectors
  \********************************************/
  vector<QuantizedBlock<D>>().swap(quantized_Y);
  vector<QuantizedBlock<D>>().swap(quantized_Cb);
  vector<QuantizedBlock<D>>().swap(quantized_Cr);


  HuffmanWeights weights_luma_AC   {};
  HuffmanWeights weights_luma_DC   {};

  huffmanGetWeightsAC(runlength_Y,  weights_luma_AC);
  huffmanGetWeightsDC(runlength_Y,  weights_luma_DC);

  HuffmanWeights weights_chroma_AC {};
  HuffmanWeights weights_chroma_DC {};

  huffmanGetWeightsAC(runlength_Cb, weights_chroma_AC);
  huffmanGetWeightsAC(runlength_Cr, weights_chroma_AC);
  huffmanGetWeightsDC(runlength_Cb, weights_chroma_DC);
  huffmanGetWeightsDC(runlength_Cr, weights_chroma_DC);

  HuffmanCodelengths codelengths_luma_DC   = generateHuffmanCodelengths(weights_luma_DC);
  HuffmanCodelengths codelengths_luma_AC   = generateHuffmanCodelengths(weights_luma_AC);
  HuffmanCodelengths codelengths_chroma_DC = generateHuffmanCodelengths(weights_chroma_DC);
  HuffmanCodelengths codelengths_chroma_AC = generateHuffmanCodelengths(weights_chroma_AC);

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

  HuffmanMap huffmap_luma_DC   = generateHuffmanMap(codelengths_luma_DC);
  HuffmanMap huffmap_luma_AC   = generateHuffmanMap(codelengths_luma_AC);
  HuffmanMap huffmap_chroma_DC = generateHuffmanMap(codelengths_chroma_DC);
  HuffmanMap huffmap_chroma_AC = generateHuffmanMap(codelengths_chroma_AC);

  OBitstream bitstream(output);

  for (size_t i = 0; i < blocks_cnt * imgs_cnt; i++) {
    encodeOneBlock(runlength_Y[i], huffmap_luma_DC, huffmap_luma_AC, bitstream);
    encodeOneBlock(runlength_Cb[i], huffmap_chroma_DC, huffmap_chroma_AC, bitstream);
    encodeOneBlock(runlength_Cr[i], huffmap_chroma_DC, huffmap_chroma_AC, bitstream);
  }

  bitstream.flush();

  return 0;
}

#endif