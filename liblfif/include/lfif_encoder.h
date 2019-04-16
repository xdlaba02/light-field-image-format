/******************************************************************************\
* SOUBOR: lfif_encoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_ENCODER_H
#define LFIF_ENCODER_H

#include "block_compress_chain.h"
#include "bitstream.h"

#include <cstdint>
#include <ostream>
#include <sstream>

template<size_t BS, size_t D>
struct LfifEncoder {
  uint16_t max_rgb_value;
  uint64_t img_dims[D+1];

  QuantTable<BS, D>      quant_table     [2];
  TraversalTable<BS, D>  traversal_table [2];
  ReferenceBlock<BS, D>  reference_block [2];
  HuffmanWeights         huffman_weight  [2][2];
  HuffmanEncoder         huffman_encoder [2][2];

  QuantTable<BS, D>     *quant_tables    [3];
  ReferenceBlock<BS, D> *reference_blocks[3];
  TraversalTable<BS, D> *traversal_tables[3];
  HuffmanWeights        *huffman_weights [3];
  HuffmanEncoder        *huffman_encoders[3];

  size_t blocks_cnt;
  size_t pixels_cnt;

  size_t input_bits;
  size_t amp_bits;
  size_t zeroes_bits;
  size_t class_bits;
  size_t max_zeroes;

  Block<INPUTTRIPLET,  BS, D> current_block;
  Block<INPUTUNIT,     BS, D> input_block;
  Block<DCTDATAUNIT,   BS, D> dct_block;
  Block<QDATAUNIT,     BS, D> quantized_block;
  Block<RunLengthPair, BS, D> runlength;
};

template<size_t BS, size_t D>
void initEncoder(LfifEncoder<BS, D> &enc) {
  enc.quant_tables[0]     = &enc.quant_table[0];
  enc.quant_tables[1]     = &enc.quant_table[1];
  enc.quant_tables[2]     = &enc.quant_table[1];

  enc.reference_blocks[0] = &enc.reference_block[0];
  enc.reference_blocks[1] = &enc.reference_block[1];
  enc.reference_blocks[2] = &enc.reference_block[1];

  enc.traversal_tables[0] = &enc.traversal_table[0];
  enc.traversal_tables[1] = &enc.traversal_table[1];
  enc.traversal_tables[2] = &enc.traversal_table[1];

  enc.huffman_weights[0]  =  enc.huffman_weight[0];
  enc.huffman_weights[1]  =  enc.huffman_weight[1];
  enc.huffman_weights[2]  =  enc.huffman_weight[1];

  enc.huffman_encoders[0] =  enc.huffman_encoder[0];
  enc.huffman_encoders[1] =  enc.huffman_encoder[1];
  enc.huffman_encoders[2] =  enc.huffman_encoder[1];

  enc.blocks_cnt = 1;
  enc.pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    enc.blocks_cnt *= ceil(enc.img_dims[i] / static_cast<double>(BS));
    enc.pixels_cnt *= enc.img_dims[i];
  }

  enc.input_bits = ceil(log2(enc.max_rgb_value));
  enc.amp_bits = ceil(log2(constpow(BS, D))) + enc.input_bits - D - (D/2) + 1;
  enc.class_bits = RunLengthPair::classBits(enc.amp_bits);
  enc.zeroes_bits = RunLengthPair::zeroesBits(enc.class_bits);
  enc.max_zeroes = constpow(2, enc.zeroes_bits);

  for (size_t y = 0; y < 2; y++) {
    enc.reference_block[y].fill(0);
    for (size_t x = 0; x < 2; x++) {
      enc.huffman_weight[y][x].clear();
    }
  }
}

template<size_t BS, size_t D>
int constructQuantizationTables(LfifEncoder<BS, D> &enc, const std::string &table_type, float quality) {
  auto scale_table = [&](const auto &table) {
    if (table_type == "DEFAULT")  return scaleByDCT<8, 2, BS>(table);
    if (table_type == "DCTDIAG")  return scaleByDCT<8, 2, BS>(table);
    if (table_type == "DCTCOPY")  return scaleByDCT<8, 2, BS>(table);
    if (table_type == "FILLDIAG") return scaleFillNear<8, 2, BS>(table);
    if (table_type == "FILLCOPY") return scaleFillNear<8, 2, BS>(table);
    return QuantTable<BS, 2>();
  };

  auto extend_table = [&](const auto &table) {
    if (table_type == "DEFAULT")  return averageDiagonalTable<BS, 2, D>(table);
    if (table_type == "DCTDIAG")  return averageDiagonalTable<BS, 2, D>(table);
    if (table_type == "DCTCOPY")  return copyTable<BS, 2, D>(table);
    if (table_type == "FILLDIAG") return averageDiagonalTable<BS, 2, D>(table);
    if (table_type == "FILLCOPY") return copyTable<BS, 2, D>(table);
    return QuantTable<BS, D>();
  };

  if (table_type == "DEFAULT" || table_type == "DCTDIAG" || table_type == "FILLDIAG" || table_type == "DCTCOPY" || table_type == "FILLCOPY") {
    enc.quant_table[0] = extend_table(scale_table(base_luma));
    enc.quant_table[1] = extend_table(scale_table(base_chroma));

    for (size_t i = 0; i < 2; i++) {
      enc.quant_table[i] = clampTable<BS, D>(applyQuality<BS, D>(enc.quant_table[i], quality), 1, 255);
    }
  }
  else if (table_type == "UNIFORM") {
    QTABLEUNIT uniform_coef = 256 - quality * 255;
    for (size_t i = 0; i < 2; i++) {
      enc.quant_table[i] = clampTable<BS, D>(uniformTable<BS, D>(uniform_coef), 1, 255);
    }
  }
  else {
    return -1;
  }

  return 0;
}

template<size_t BS, size_t D>
int constructTraversalTables(LfifEncoder<BS, D> &enc, const std::string &table_type) {
  if (table_type == "DEFAULT" || table_type == "REFERENCE") {
    for (size_t i = 0; i < 2; i++) {
      enc.traversal_table[i]
      .constructByReference(enc.reference_block[i]);
    }
  }
  else if (table_type == "ZIGZAG") {
    for (size_t i = 0; i < 2; i++) {
      enc.traversal_table[i]
      .constructZigzag();
    }
  }
  else if (table_type == "QTABLE") {
    for (size_t i = 0; i < 2; i++) {
      enc.traversal_table[i]
      .constructByQuantTable(enc.quant_table[i]);
    }
  }
  else if (table_type == "RADIUS") {
    for (size_t i = 0; i < 2; i++) {
      enc.traversal_table[i]
      .constructByRadius();
    }
  }
  else if (table_type == "DIAGONALS") {
    for (size_t i = 0; i < 2; i++) {
      enc.traversal_table[i]
      .constructByDiagonals();
    }
  }
  else if (table_type == "HYPERBOLOID") {
    for (size_t i = 0; i < 2; i++) {
      enc.traversal_table[i]
      .constructByHyperboloid();
    }
  }
  else {
    return -1;
  }

  return 0;
}

template<size_t BS, size_t D>
struct performScan {
  template<typename INPUTF, typename PERFF>
  performScan(LfifEncoder<BS, D> &enc, INPUTF &&input, PERFF &&func) {
    auto outputF = [&](size_t index, const auto &value) {
      enc.current_block[index] = value;
    };

    for (size_t img = 0; img < enc.img_dims[D]; img++) {
      auto inputF = [&](size_t index) {
        return input(img * enc.pixels_cnt + index);
      };

      for (size_t block = 0; block < enc.blocks_cnt; block++) {
        getBlock<BS, D>(inputF, block, enc.img_dims, outputF);


        for (size_t channel = 0; channel < 3; channel++) {
          for (size_t i = 0; i < constpow(BS, D); i++) {
            enc.input_block[i] = enc.current_block[i][channel];
          }

          func(channel);
        }
      }
    }
  }
};

template<size_t BS, size_t D>
struct referenceScan {
  template<typename F>
  referenceScan(LfifEncoder<BS, D> &enc, F &&input) {
    auto perform = [&](size_t channel) {
      forwardDiscreteCosineTransform<BS, D>(enc.input_block,      enc.dct_block);
                            quantize<BS, D>(enc.dct_block,        enc.quantized_block, *enc.quant_tables[channel]);
                 addToReferenceBlock<BS, D>(enc.quantized_block, *enc.reference_blocks[channel]);
    };

    performScan(enc, input, perform);
  }
};

template<size_t BS, size_t D>
struct huffmanScan {
  template<typename F>
  huffmanScan(LfifEncoder<BS, D> &enc, F &&input) {
    QDATAUNIT previous_DC [3] {};

    auto perform = [&](size_t channel) {
      forwardDiscreteCosineTransform<BS, D>(enc.input_block,      enc.dct_block);
                            quantize<BS, D>(enc.dct_block,        enc.quantized_block,         *enc.quant_tables[channel]);
                        diffEncodeDC<BS, D>(enc.quantized_block,  previous_DC[channel]);
                            traverse<BS, D>(enc.quantized_block, *enc.traversal_tables[channel]);
                     runLengthEncode<BS, D>(enc.quantized_block,  enc.runlength,                enc.max_zeroes);
                   huffmanAddWeights<BS, D>(enc.runlength,        enc.huffman_weights[channel], enc.class_bits);
    };

    performScan(enc, input, perform);
  }
};

template<size_t BS, size_t D>
struct outputScan {
  template<typename F>
  outputScan(LfifEncoder<BS, D> &enc, F &&input, std::ostream &output) {
    QDATAUNIT previous_DC [3] {};
    OBitstream bitstream      {};

    bitstream.open(&output);

    auto perform = [&](size_t channel) {
      forwardDiscreteCosineTransform<BS, D>(enc.input_block,      enc.dct_block);
                            quantize<BS, D>(enc.dct_block,        enc.quantized_block,          *enc.quant_tables[channel]);
                        diffEncodeDC<BS, D>(enc.quantized_block,  previous_DC[channel]);
                            traverse<BS, D>(enc.quantized_block, *enc.traversal_tables[channel]);
                     runLengthEncode<BS, D>(enc.quantized_block,  enc.runlength,                 enc.max_zeroes);
                      encodeToStream<BS, D>(enc.runlength,        enc.huffman_encoders[channel], bitstream, enc.class_bits);
    };

    performScan(enc, input, perform);

    bitstream.flush();
  }
};

template<size_t BS, size_t D>
void constructHuffmanTables(LfifEncoder<BS, D> &enc) {
  for (size_t y = 0; y < 2; y++) {
    for (size_t x = 0; x < 2; x++) {
      enc.huffman_encoder[y][x]
      . generateFromWeights(enc.huffman_weight[y][x]);
    }
  }
}

template<size_t BS, size_t D>
void writeHeader(LfifEncoder<BS, D> &enc, std::ostream &output) {
  output << "LFIF-" << D << "D\n";
  output << BS << "\n";

  writeValueToStream<uint16_t>(enc.max_rgb_value, output);

  for (size_t i = 0; i < D + 1; i++) {
    writeValueToStream<uint64_t>(enc.img_dims[i], output);
  }

  for (size_t i = 0; i < 2; i++) {
    writeToStream<BS, D>(enc.quant_table[i], output);
  }

  for (size_t i = 0; i < 2; i++) {
    enc.traversal_table[i]
    . writeToStream(output);
  }

  for (size_t y = 0; y < 2; y++) {
    for (size_t x = 0; x < 2; x++) {
      enc.huffman_encoder[y][x]
      . writeToStream(output);
    }
  }
}

template<size_t BS, size_t D>
struct lfifCompress {
  template<typename F>
  lfifCompress(F &&input, const uint64_t img_dims[D+1], float quality, uint16_t max_rgb_value, std::ostream &output) {
    OBitstream             bitstream              {};

    QuantTable<BS, D>      quant_table     [2]    {};
    ReferenceBlock<BS, D>  reference_block [2]    {};
    TraversalTable<BS, D>  traversal_table [2]    {};
    HuffmanWeights         huffman_weight  [2][2] {};
    HuffmanEncoder         huffman_encoder [2][2] {};

    QDATAUNIT              previous_DC     [3]    {};
    QuantTable<BS, D>     *quant_tables    [3]    {};
    ReferenceBlock<BS, D> *reference_blocks[3]    {};
    TraversalTable<BS, D> *traversal_tables[3]    {};
    HuffmanWeights        *huffman_weights [3]    {};
    HuffmanEncoder        *huffman_encoders[3]    {};

    size_t blocks_cnt  {};
    size_t pixels_cnt  {};

    size_t input_bits    {};
    size_t amp_bits    {};
    size_t zeroes_bits {};
    size_t class_bits  {};
    size_t max_zeroes  {};

    Block<std::array<INPUTUNIT, 3>, BS, D> current_block   {};
    Block<INPUTUNIT,                BS, D> input_block     {};
    Block<DCTDATAUNIT,              BS, D> dct_block       {};
    Block<QDATAUNIT,                BS, D> quantized_block {};
    Block<RunLengthPair,            BS, D> runlength       {};

    auto outputF = [&](size_t index, const auto &value) {
      current_block[index] = value;
    };

    quant_tables[0]     = &quant_table[0];
    quant_tables[1]     = &quant_table[1];
    quant_tables[2]     = &quant_table[1];

    reference_blocks[0] = &reference_block[0];
    reference_blocks[1] = &reference_block[1];
    reference_blocks[2] = &reference_block[1];

    traversal_tables[0] = &traversal_table[0];
    traversal_tables[1] = &traversal_table[1];
    traversal_tables[2] = &traversal_table[1];

    huffman_weights[0]  =  huffman_weight[0];
    huffman_weights[1]  =  huffman_weight[1];
    huffman_weights[2]  =  huffman_weight[1];

    huffman_encoders[0] =  huffman_encoder[0];
    huffman_encoders[1] =  huffman_encoder[1];
    huffman_encoders[2] =  huffman_encoder[1];

    blocks_cnt = 1;
    pixels_cnt = 1;

    for (size_t i = 0; i < D; i++) {
      blocks_cnt *= ceil(img_dims[i]/static_cast<double>(BS));
      pixels_cnt *= img_dims[i];
    }

    input_bits = ceil(log2(max_rgb_value));
    amp_bits = ceil(log2(constpow(BS, D))) + input_bits - D - (D/2) + 1;
    class_bits = RunLengthPair::classBits(amp_bits);
    zeroes_bits = RunLengthPair::zeroesBits(class_bits);
    max_zeroes = constpow(2, zeroes_bits);

    quant_table[0] = averageDiagonalTable<BS, 2, D>(scaleByDCT<8, 2, BS>(base_luma));
    quant_table[1] = averageDiagonalTable<BS, 2, D>(scaleByDCT<8, 2, BS>(base_chroma));

    for (size_t i = 0; i < 2; i++) {
      quant_table[i] = clampTable<BS, D>(applyQuality<BS, D>(quant_table[i], quality), 1, 255);
    }

    for (size_t img = 0; img < img_dims[D]; img++) {
      auto inputF = [&](size_t index) {
        return input(img * pixels_cnt + index);
      };

      for (size_t block = 0; block < blocks_cnt; block++) {
        getBlock<BS, D>(inputF, block, img_dims, outputF);

        for (size_t channel = 0; channel < 3; channel++) {
          for (size_t i = 0; i < constpow(BS, D); i++) {
            input_block[i] = current_block[i][channel];
          }

          forwardDiscreteCosineTransform<BS, D>(input_block, dct_block);
                                quantize<BS, D>(dct_block, quantized_block, *quant_tables[channel]);
                     addToReferenceBlock<BS, D>(quantized_block, *reference_blocks[channel]);
        }
      }
    }

    for (size_t i = 0; i < 2; i++) {
      traversal_table[i]
      . constructByReference(reference_block[i]);
    }

    previous_DC[0] = 0;
    previous_DC[1] = 0;
    previous_DC[2] = 0;

    for (size_t img = 0; img < img_dims[D]; img++) {
      auto inputF = [&](size_t index) {
        return input(img * pixels_cnt + index);
      };

      for (size_t block = 0; block < blocks_cnt; block++) {
        getBlock<BS, D>(inputF, block, img_dims, outputF);

        for (size_t channel = 0; channel < 3; channel++) {
          for (size_t i = 0; i < constpow(BS, D); i++) {
            input_block[i] = current_block[i][channel];
          }

          forwardDiscreteCosineTransform<BS, D>(input_block, dct_block);
                                quantize<BS, D>(dct_block, quantized_block, *quant_tables[channel]);
                            diffEncodeDC<BS, D>(quantized_block, previous_DC[channel]);
                                traverse<BS, D>(quantized_block, *traversal_tables[channel]);
                         runLengthEncode<BS, D>(quantized_block, runlength, max_zeroes);
                       huffmanAddWeights<BS, D>(runlength, huffman_weights[channel], class_bits);
        }
      }
    }

    for (size_t i = 0; i < 2; i++) {
      writeToStream<BS, D>(quant_table[i], output);
    }

    for (size_t i = 0; i < 2; i++) {
      traversal_table[i]
      . writeToStream(output);
    }

    for (size_t y = 0; y < 2; y++) {
      for (size_t x = 0; x < 2; x++) {
        huffman_encoder[y][x]
        . generateFromWeights(huffman_weight[y][x])
        . writeToStream(output);
      }
    }

    bitstream.open(&output);

    previous_DC[0] = 0;
    previous_DC[1] = 0;
    previous_DC[2] = 0;

    for (size_t img = 0; img < img_dims[D]; img++) {
      auto inputF = [&](size_t index) {
        return input(img * pixels_cnt + index);
      };

      for (size_t block = 0; block < blocks_cnt; block++) {
        getBlock<BS, D>(inputF, block, img_dims, outputF);

        for (size_t channel = 0; channel < 3; channel++) {
          for (size_t i = 0; i < constpow(BS, D); i++) {
            input_block[i] = current_block[i][channel];
          }

          forwardDiscreteCosineTransform<BS, D>(input_block, dct_block);
                                quantize<BS, D>(dct_block, quantized_block, *quant_tables[channel]);
                            diffEncodeDC<BS, D>(quantized_block, previous_DC[channel]);
                                traverse<BS, D>(quantized_block, *traversal_tables[channel]);
                         runLengthEncode<BS, D>(quantized_block, runlength, max_zeroes);
                          encodeToStream<BS, D>(runlength, huffman_encoders[channel], bitstream, class_bits);
        }
      }
    }

    bitstream.flush();
  }
};


#endif
