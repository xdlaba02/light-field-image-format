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
#include "block_decompress_chain.h"
#include "cabac_contexts.h"
#include "bitstream.h"
#include "predict.h"

#include <cstdint>
#include <ostream>
#include <sstream>

/**
* @brief Base structure for encoding an image.
*/
template<size_t BS, size_t D>
struct LfifEncoder {
  uint8_t color_depth;    /**< @brief Number of bits per sample used by each decoded channel.*/
  std::array<uint64_t, D + 1> img_dims; /**< @brief Dimensions of a encoded image + image count. The multiple of all values should be equal to number of pixels in encoded image.*/

  std::array<uint64_t, D> img_dims_unaligned;
  std::array<uint64_t, D> img_dims_aligned;

  std::array<uint64_t, D + 1> img_stride_unaligned;
  std::array<uint64_t, D + 1> img_stride_aligned;

  std::array<size_t, D>     block_dims; /**< @brief Dimensions of an encoded image in blocks. The multiple of all values should be equal to number of blocks in encoded image.*/
  std::array<size_t, D + 1> block_stride;

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
  enc.img_stride_unaligned[0] = 1;
  enc.img_stride_aligned[0]   = 1;
  enc.block_stride[0]         = 1;

  for (size_t i = 0; i < D; i++) {
    enc.block_dims[i]       = ceil(enc.img_dims[i] / static_cast<double>(BS));
    enc.block_stride[i + 1] = enc.block_stride[i] * enc.block_dims[i];

    enc.img_dims_unaligned[i]       = enc.img_dims[i];
    enc.img_stride_unaligned[i + 1] = enc.img_stride_unaligned[i] * enc.img_dims_unaligned[i];

    enc.img_dims_aligned[i]       = ceil(enc.block_dims[i] * BS);
    enc.img_stride_aligned[i + 1] = enc.img_stride_aligned[i] * enc.img_dims_aligned[i];
  }

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
  auto outputF = [&](const std::array<size_t, D> &block_pos, const auto &value) {
    enc.current_block[get_index<BS, D>(block_pos)] = value;
  };

  for (size_t img = 0; img < enc.img_dims[D]; img++) {
    auto inputF = [&](const std::array<size_t, D> &img_pos) {
      size_t img_index {};

      for (size_t i { 0 }; i < D; i++) {
        img_index += img_pos[i] * enc.img_stride_unaligned[i];
      }

      img_index += img * enc.img_stride_unaligned[D];

      return input(img_index);
    };

    iterate_dimensions<D>(enc.block_dims, [&](const std::array<size_t, D> &pos_block) {
      getBlock<BS, D>(inputF, pos_block, enc.img_dims_unaligned, outputF);

      for (size_t channel = 0; channel < 3; channel++) {
        for (size_t i = 0; i < constpow(BS, D); i++) {
          enc.input_block[i] = enc.current_block[i][channel];
        }

        func(img, channel, pos_block);
      }
    });
  }
}

/**
* @brief Function which performs scan for linearization optimization.
* @param enc The encoder structure.
* @param input Callback function specified by client with signature T input(size_t index), where T is pixel/sample type.
*/
template<size_t BS, size_t D, typename F>
void referenceScan(LfifEncoder<BS, D> &enc, F &&input) {
  auto perform = [&](size_t, size_t channel, const std::array<size_t, D> &) {
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

  auto perform = [&](size_t, size_t channel, const std::array<size_t, D> &) {
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

  writeValueToStream<uint8_t>(enc.use_huffman, output);

  if (enc.use_huffman) {
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
}

/**
* @brief Function which encodes the image to the stream.
* @param enc The encoder structure.
* @param input Callback function specified by client with signature T input(size_t index), where T is pixel/sample type.
* @param output Output stream to which the image shall be encoded.
*/
template<size_t BS, size_t D, typename F>
void outputScanHuffman_RUNLENGTH(LfifEncoder<BS, D> &enc, F &&input, std::ostream &output) {
  QDATAUNIT previous_DC [3] {};
  OBitstream bitstream      {};

  bitstream.open(&output);

  auto perform = [&](size_t, size_t channel, const std::array<size_t, D> &) {
    forwardDiscreteCosineTransform<BS, D>(enc.input_block,      enc.dct_block);
                          quantize<BS, D>(enc.dct_block,        enc.quantized_block,          *enc.quant_tables[channel]);
                      diffEncodeDC<BS, D>(enc.quantized_block,  previous_DC[channel]);
                          traverse<BS, D>(enc.quantized_block, *enc.traversal_tables[channel]);
                   runLengthEncode<BS, D>(enc.quantized_block,  enc.runlength,                 enc.max_zeroes);
           encodeHuffman_RUNLENGTH<BS, D>(enc.runlength,        enc.huffman_encoders[channel], bitstream, enc.class_bits);
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
void outputScanCABAC_DIAGONAL(LfifEncoder<BS, D> &enc, F &&input, std::ostream &output) {
  std::array<         std::vector<size_t>, D * (BS - 1) + 1> scan_table {};
  std::array<CABACContextsDIAGONAL<BS, D>, 2>                contexts   {};
  std::array<      std::vector<INPUTUNIT>, 3>                decoded    {};
  OBitstream                                                 bitstream  {};
  CABACEncoder                                               cabac      {};
  size_t                                                     threshold  {};
  Block<INPUTUNIT,     BS, D>                                prediction_block {};

  threshold = (D * (BS - 1) + 1) / 2;

  for (size_t i = 0; i < constpow(BS, D); i++) {
    size_t diagonal { 0 };
    for (size_t j = i; j; j /= BS) {
      diagonal += j % BS;
    }

    scan_table[diagonal].push_back(i);
  }

  for (size_t i = 0; i < 3; i++) {
    decoded[i].resize(enc.img_stride_aligned[D]);
  }

  bitstream.open(&output);
  cabac.init(bitstream);

  uint64_t prediction_type {};

  auto inputF = [&](const std::array<size_t, D> &pos) -> const auto & {
    return enc.input_block[get_index<BS, D>(pos)];
  };

  auto perform = [&](size_t image, size_t channel, const std::array<size_t, D> &block) {
    auto outputF = [&](const std::array<size_t, D> &pos, const auto &value) {
      size_t img_index {};

      for (size_t i { 0 }; i < D; i++) {
        img_index += pos[i] * enc.img_stride_unaligned[i];
      }

      decoded[channel][img_index] = value;
    };

    auto predInputF = [&](const std::array<int64_t, D> &pos) {
      int64_t src_idx {};

      for (size_t d { 0 }; d < D; d++) {
        /*
        if ((position[d] < 0) && !neighbours_prev[d]) {
          position[d] = 0;
        }
        else if ((position[d] > static_cast<int64_t>(BS - 1)) && !neighbours_next[d]) {
          position[d] = BS - 1;
        }
        */

        src_idx += pos[d] * enc.img_stride_unaligned[d];
      }

      return decoded[channel][src_idx];
    };

    if (channel == 0) {
      //prediction_type = find_best_prediction_type<BS, D>(enc.input_block, predInputF);
      //encodePredictionType(prediction_type, cabac, contexts[0]);
    }

    //                       predict<BS, D>(prediction_block,    prediction_type,      predInputF);
    //               applyPrediction<BS, D>(enc.input_block,     prediction_block                                                   );
    forwardDiscreteCosineTransform<BS, D>(enc.input_block,     enc.dct_block                                                      );
                          quantize<BS, D>(enc.dct_block,       enc.quantized_block, *enc.quant_tables[channel]                    );
    //                    dequantize<BS, D>(enc.quantized_block, enc.dct_block,       *enc.quant_tables[channel]                    );
    //inverseDiscreteCosineTransform<BS, D>(enc.dct_block,       enc.input_block                                                    );
    //              disusePrediction<BS, D>(enc.input_block,     prediction_block                                                   );
    //                      putBlock<BS, D>(inputF,              block,                enc.img_dims_aligned,   outputF              );
              encodeCABAC_DIAGONAL<BS, D>(enc.quantized_block, cabac,                contexts[channel != 0], threshold, scan_table);

  };

  performScan(enc, input, perform);

  cabac.terminate();
  bitstream.flush();
}

template<size_t BS, size_t D, typename F>
void outputScanCABAC_RUNLENGTH(LfifEncoder<BS, D> &enc, F &&input, std::ostream &output) {
  std::array<CABACContextsRUNLENGTH<BS, D>, 3> contexts    {};
  std::array<QDATAUNIT,                     3> previous_DC {};
  OBitstream                                   bitstream   {};
  CABACEncoder                                 cabac       {};

  bitstream.open(&output);
  cabac.init(bitstream);

  auto perform = [&](size_t, size_t channel, const std::array<size_t, D> &) {
    forwardDiscreteCosineTransform<BS, D>(enc.input_block,      enc.dct_block);
                          quantize<BS, D>(enc.dct_block,        enc.quantized_block, *enc.quant_tables[channel]);
                      diffEncodeDC<BS, D>(enc.quantized_block,  previous_DC[channel]);
                          traverse<BS, D>(enc.quantized_block, *enc.traversal_tables[channel]);
                   runLengthEncode<BS, D>(enc.quantized_block,  enc.runlength,        enc.max_zeroes);
             encodeCABAC_RUNLENGTH<BS, D>(enc.runlength,        cabac,                contexts[channel], enc.class_bits);
  };

  performScan(enc, input, perform);

  cabac.terminate();
  bitstream.flush();
}

template<size_t BS, size_t D, typename F>
void outputScanCABAC_JPEG(LfifEncoder<BS, D> &enc, F &&input, std::ostream &output) {
  std::array<CABACContextsJPEG<BS, D>, 3> contexts    { enc.amp_bits, enc.amp_bits, enc.amp_bits };
  std::array<QDATAUNIT,                3> previous_DC {};
  std::array<QDATAUNIT,                3> previous_DC_diff {};
  OBitstream                              bitstream   {};
  CABACEncoder                            cabac       {};

  bitstream.open(&output);
  cabac.init(bitstream);

  auto perform = [&](size_t, size_t channel, const std::array<size_t, D> &) {
    forwardDiscreteCosineTransform<BS, D>(enc.input_block,      enc.dct_block);
                          quantize<BS, D>(enc.dct_block,        enc.quantized_block, *enc.quant_tables[channel]);
                      diffEncodeDC<BS, D>(enc.quantized_block,  previous_DC[channel]);
                          traverse<BS, D>(enc.quantized_block, *enc.traversal_tables[channel]);
                  encodeCABAC_JPEG<BS, D>(enc.quantized_block,  cabac,                contexts[channel], previous_DC_diff[channel], enc.amp_bits);
  };

  performScan(enc, input, perform);

  cabac.terminate();
  bitstream.flush();
}

template<size_t BS, size_t D, typename F>
void outputScanCABAC_H264(LfifEncoder<BS, D> &enc, F &&input, std::ostream &output) {
  std::array<CABACContextsH264<BS, D>, 3> contexts    {};
  std::array<QDATAUNIT,                3> previous_DC {};
  OBitstream                              bitstream   {};
  CABACEncoder                            cabac       {};

  bitstream.open(&output);
  cabac.init(bitstream);

  auto perform = [&](size_t, size_t channel, const std::array<size_t, D> &) {
    forwardDiscreteCosineTransform<BS, D>(enc.input_block,      enc.dct_block);
                          quantize<BS, D>(enc.dct_block,        enc.quantized_block, *enc.quant_tables[channel]);
                      diffEncodeDC<BS, D>(enc.quantized_block,  previous_DC[channel]);
                          traverse<BS, D>(enc.quantized_block, *enc.traversal_tables[channel]);
                  encodeCABAC_H264<BS, D>(enc.quantized_block,  cabac,                contexts[channel]);
  };

  performScan(enc, input, perform);

  cabac.terminate();
  bitstream.flush();
}

#endif
