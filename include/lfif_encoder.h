/*******************************************************\
* SOUBOR: lfif_encoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#ifndef LFIF_ENCODER_H
#define LFIF_ENCODER_H

#include <iostream>

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

void encodeOnePair(const RunLengthPair &pair, const HuffmanMap &map, OBitstream &stream);

HuffmanClass huffmanClass(RunLengthAmplitudeUnit amplitude);
HuffmanSymbol huffmanSymbol(const RunLengthPair &pair);

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
inline constexpr QuantTable<D> baseQuantTable() {
  QuantTable<D> quant_table {};

  for (size_t i = 0; i < quant_table.size(); i++) {
    size_t sum = 0;
    size_t max = 0;
    for (size_t j = 0; j < D; j++) {
      size_t coord = (i % constpow(8, j+1)) / constpow(8, j);
      sum += coord * coord;
      if (coord > max) {
        max = coord;
      }
    }
    quant_table[i] = clamp(((sqrt(sum)+1) * max) + 10, 1., 255.);
  }

  return quant_table;
}

template<size_t D>
inline QuantTable<D> scaleQuantTable(QuantTable<D> quant_table, const QualityUnit quality) {
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

#endif
