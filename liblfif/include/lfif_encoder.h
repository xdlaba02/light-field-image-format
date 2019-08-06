/**
* @file lfif_encoder.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for encoding an image.
*/

#ifndef LFIF_ENCODER_H
#define LFIF_ENCODER_H

#include "block_compress_chain.h"
#include "cabac_contexts.h"
#include "bitstream.h"

#include <cstdint>
#include <ostream>
#include <sstream>

/**
* @brief Base structure for encoding an image.
*/
template<size_t BS, size_t D>
struct LfifEncoder {
  uint8_t color_depth;    /**< @brief Number of bits per sample used by each decoded channel.*/
  uint64_t img_dims[D+1]; /**< @brief Dimensions of a decoded image + image count. The multiple of all values should be equal to number of pixels in encoded image.*/

  bool use_huffman; /**< @brief Huffman encoding will be used when this is true.*/

  QuantTable<BS, D>      quant_table     [2];    /**< @brief Quantization matrices for luma and chroma.*/
  TraversalTable<BS, D>  traversal_table [2];    /**< @brief Traversal matrices for luma and chroma.*/
  ReferenceBlock<BS, D>  reference_block [2];    /**< @brief Reference blocks for luma and chroma to be used for traversal optimization.*/
  HuffmanWeights         huffman_weight  [2][2]; /**< @brief Weigth maps for luma and chroma and also for the DC and AC coefficients to be used for optimal Huffman encoding.*/
  HuffmanEncoder         huffman_encoder [2][2]; /**< @brief Huffman encoders for luma and chroma and also for the DC and AC coefficients.*/

  QuantTable<BS, D>     *quant_tables    [3]; /**< @brief Pointer to quantization matrix used by each channel.*/
  ReferenceBlock<BS, D> *reference_blocks[3]; /**< @brief Pointer to reference block used by each channel.*/
  TraversalTable<BS, D> *traversal_tables[3]; /**< @brief Pointer to traversal matrix used by each channel.*/
  HuffmanWeights        *huffman_weights [3]; /**< @brief Pointer to weight maps used by each channel.*/
  HuffmanEncoder        *huffman_encoders[3]; /**< @brief Pointer to Huffman encoders used by each channel.*/

  size_t blocks_cnt; /**< @brief Number of blocks in the encoded image.*/
  size_t pixels_cnt; /**< @brief Number of pixels in the encoded image.*/

  size_t amp_bits;    /**< @brief Number of bits sufficient to contain maximum DCT coefficient.*/
  size_t zeroes_bits; /**< @brief Number of bits sufficient to contain run-length of zero coefficients.*/
  size_t class_bits;  /**< @brief Number of bits sufficient to contain number of bits of maximum DCT coefficient.*/
  size_t max_zeroes;  /**< @brief Maximum length of zero coefficient run-length.*/

  Block<INPUTTRIPLET,  BS, D> current_block;   /**< @brief Buffer for caching the block of pixels before encoding.*/
  Block<INPUTUNIT,     BS, D> input_block;     /**< @brief Buffer for caching the block of input samples.*/
  Block<DCTDATAUNIT,   BS, D> dct_block;       /**< @brief Buffer for caching the block of DCT coefficients.*/
  Block<QDATAUNIT,     BS, D> quantized_block; /**< @brief Buffer for caching the block of quantized coefficients.*/
  Block<RunLengthPair, BS, D> runlength;       /**< @brief Buffer for caching the block of run-length pairs.*/
};

/**
* @brief Function which (re)initializes the encoder structure.
* @param enc The encoder structure.
*/
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

  enc.amp_bits = ceil(log2(constpow(BS, D))) + enc.color_depth - D - (D/2);
  enc.class_bits = RunLengthPair::classBits(enc.amp_bits);
  enc.zeroes_bits = RunLengthPair::zeroesBits(enc.class_bits);
  enc.max_zeroes = constpow(2, enc.zeroes_bits) - 1;

  for (size_t y = 0; y < 2; y++) {
    enc.reference_block[y].fill(0);
    for (size_t x = 0; x < 2; x++) {
      enc.huffman_weight[y][x].clear();
    }
  }
}

/**
* @brief Function which initializes quantization matrices.
* @param enc The encoder structure.
* @param table_type Type of the tables to be initialized.
* @param quality Encoding quality from 1 to 100.
* @return Nonzero if requested unknown table type.
*/
template<size_t BS, size_t D>
int constructQuantizationTables(LfifEncoder<BS, D> &enc, const std::string &table_type, float quality) {
  auto scale_table = [&](const auto &table) {
    if (table_type == "DCTDIAG")  return scaleByDCT<8, 2, BS>(table);
    if (table_type == "DCTCOPY")  return scaleByDCT<8, 2, BS>(table);
    if (table_type == "FILLDIAG") return scaleFillNear<8, 2, BS>(table);
    if (table_type == "FILLCOPY") return scaleFillNear<8, 2, BS>(table);
    return QuantTable<BS, 2>();
  };

  auto extend_table = [&](const auto &table) {
    if (table_type == "DCTDIAG")  return averageDiagonalTable<BS, 2, D>(table);
    if (table_type == "DCTCOPY")  return copyTable<BS, 2, D>(table);
    if (table_type == "FILLDIAG") return averageDiagonalTable<BS, 2, D>(table);
    if (table_type == "FILLCOPY") return copyTable<BS, 2, D>(table);
    return QuantTable<BS, D>();
  };

  if (table_type == "DCTDIAG" || table_type == "FILLDIAG" || table_type == "DCTCOPY" || table_type == "FILLCOPY") {
    enc.quant_table[0] = extend_table(scale_table(base_luma));
    enc.quant_table[1] = extend_table(scale_table(base_chroma));
  }
  else if (table_type == "DEFAULT" || table_type == "UNIFORM") {
    for (size_t i = 0; i < 2; i++) {
      enc.quant_table[i] = uniformTable<BS, D>(50);
    }
  }
  else {
    return -1;
  }

  for (size_t i = 0; i < 2; i++) {
    enc.quant_table[i] = clampTable<BS, D>(applyQuality<BS, D>(enc.quant_table[i], quality), 1, 255);
  }

  return 0;
}

/**
* @brief Function which performs arbitrary scan of an image. This function prepares a block into the encoding structure buffers.
* @param enc The encoder structure.
* @param input Callback function specified by client with signature T input(size_t index), where T is pixel/sample type.
* @param func Callback function which performs action on every block with signature void func(size_t channel), where channel is channel from which the extracted block is.
*/
template<size_t BS, size_t D, typename INPUTF, typename PERFF>
void performScan(LfifEncoder<BS, D> &enc, INPUTF &&input, PERFF &&func) {
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

/**
* @brief Function which performs scan for linearization optimization.
* @param enc The encoder structure.
* @param input Callback function specified by client with signature T input(size_t index), where T is pixel/sample type.
*/
template<size_t BS, size_t D, typename F>
void referenceScan(LfifEncoder<BS, D> &enc, F &&input) {
  auto perform = [&](size_t channel) {
    forwardDiscreteCosineTransform<BS, D>(enc.input_block,      enc.dct_block);
                          quantize<BS, D>(enc.dct_block,        enc.quantized_block, *enc.quant_tables[channel]);
               addToReferenceBlock<BS, D>(enc.quantized_block, *enc.reference_blocks[channel]);
  };

  performScan(enc, input, perform);
}

/**
* @brief Function which initializes linearization matrices.
* @param enc The encoder structure.
* @param table_type Type of the tables to be initialized.
* @return Nonzero if requested unknown table type.
*/
template<size_t BS, size_t D>
int constructTraversalTables(LfifEncoder<BS, D> &enc, const std::string &table_type) {
  if (table_type == "REFERENCE") {
    for (size_t i = 0; i < 2; i++) {
      enc.traversal_table[i]
      .constructByReference(enc.reference_block[i]);
    }
  }
  else if (table_type == "DEFAULT" || table_type == "ZIGZAG") {
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

/**
* @brief Function which performs scan for huffman weights.
* @param enc The encoder structure.
* @param input Callback function specified by client with signature T input(size_t index), where T is pixel/sample type.
*/
template<size_t BS, size_t D, typename F>
void huffmanScan(LfifEncoder<BS, D> &enc, F &&input) {
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

/**
* @brief Function which initializes Huffman encoder from weights.
* @param enc The encoder structure.
*/
template<size_t BS, size_t D>
void constructHuffmanTables(LfifEncoder<BS, D> &enc) {
  for (size_t y = 0; y < 2; y++) {
    for (size_t x = 0; x < 2; x++) {
      enc.huffman_encoder[y][x]
      . generateFromWeights(enc.huffman_weight[y][x]);
    }
  }
}

/**
* @brief Function which writes tables to stream.
* @param enc The encoder structure.
* @param output Stream to which data will be written.
*/
template<size_t BS, size_t D>
void writeHeader(LfifEncoder<BS, D> &enc, std::ostream &output) {
  output << "LFIF-" << D << "D\n";
  output << BS << "\n";

  writeValueToStream<uint8_t>(enc.color_depth, output);

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

  writeValueToStream<uint8_t>(enc.use_huffman, output);

  if (enc.use_huffman) {
    for (size_t y = 0; y < 2; y++) {
      for (size_t x = 0; x < 2; x++) {
        enc.huffman_encoder[y][x]
        . writeToStream(output);
      }
    }
  }
}

/**
* @brief Function which encodes the image to the stream.
* @param enc The encoder structure.
* @param input Callback function specified by client with signature T input(size_t index), where T is pixel/sample type.
* @param output Output stream to which the image shall be encoded.
*/
template<size_t BS, size_t D, typename F>
void outputScanHuffman(LfifEncoder<BS, D> &enc, F &&input, std::ostream &output) {
  QDATAUNIT previous_DC [3] {};
  OBitstream bitstream      {};

  bitstream.open(&output);

  auto perform = [&](size_t channel) {
    forwardDiscreteCosineTransform<BS, D>(enc.input_block,      enc.dct_block);
                          quantize<BS, D>(enc.dct_block,        enc.quantized_block,          *enc.quant_tables[channel]);
                      diffEncodeDC<BS, D>(enc.quantized_block,  previous_DC[channel]);
                          traverse<BS, D>(enc.quantized_block, *enc.traversal_tables[channel]);
                   runLengthEncode<BS, D>(enc.quantized_block,  enc.runlength,                 enc.max_zeroes);
             encodeToStreamHuffman<BS, D>(enc.runlength,        enc.huffman_encoders[channel], bitstream, enc.class_bits);
  };

  performScan(enc, input, perform);

  bitstream.flush();
}

/**
* @brief Function which encodes the image to the stream with CABAC.
* @param enc The encoder structure.
* @param input Callback function specified by client with signature T input(size_t index), where T is pixel/sample type.
* @param output Output stream to which the image shall be encoded.
*/
template<size_t BS, size_t D, typename F>
void outputScanCABAC(LfifEncoder<BS, D> &enc, F &&input, std::ostream &output) {
  CABACContextsDIAGONAL<BS, D> contexts    [3] {};
  QDATAUNIT            previous_DC [3] {};
  OBitstream           bitstream       {};
  CABACEncoder         cabac           {};
  size_t threshold { (D * (BS - 1) + 1) / 2 };

  bitstream.open(&output);
  cabac.init(bitstream);

  auto perform = [&](size_t channel) {
    forwardDiscreteCosineTransform<BS, D>(enc.input_block,      enc.dct_block);
                          quantize<BS, D>(enc.dct_block,        enc.quantized_block,          *enc.quant_tables[channel]);
                      diffEncodeDC<BS, D>(enc.quantized_block,  previous_DC[channel]);
               encodeCABACDIAGONAL<BS, D>(enc.quantized_block,  cabac,                         contexts[channel], threshold);
  };

  performScan(enc, input, perform);

  cabac.terminate();
  bitstream.flush();
}

#include <iostream>
#include <iomanip>
#include <map>


template<size_t BS, size_t D, typename F>
void dummyScan(LfifEncoder<BS, D> &enc, F &&input) {
  QDATAUNIT            previous_DC [3] {};

  Block<int64_t, BS, D> sum {};
  Block<uint64_t, BS, D> shifted_abs_sum {};
  Block<double, BS, D>  variance {};

  size_t cnt { 1 };

  auto perform = [&](size_t channel) {
    forwardDiscreteCosineTransform<BS, D>(enc.input_block,      enc.dct_block);
                          quantize<BS, D>(enc.dct_block,        enc.quantized_block,          *enc.quant_tables[channel]);
                      diffEncodeDC<BS, D>(enc.quantized_block,  previous_DC[channel]);

    if (channel == 0) {
      for (size_t i = 0; i < constpow(BS, D); i++) {
        QDATAUNIT shifted_abs_value = std::abs(enc.quantized_block[i] - ((double)sum[i] / cnt));
        variance[i] += constpow(shifted_abs_value - ((double)shifted_abs_sum[i] / cnt), 2);
        shifted_abs_sum[i] += shifted_abs_value;

        sum[i] += enc.quantized_block[i];
      }
      cnt++;
    }

  };

  performScan(enc, input, perform);

  for (size_t y = 0; y < BS; y++) {
    for (size_t x = 0; x < BS; x++) {
      std::cout << std::fixed << std::setprecision(3) << std::setw(5) << (uint64_t)(variance[y * BS + x] / enc.blocks_cnt) << " ";
    }
    std::cout << '\n';
  }
}

#endif
