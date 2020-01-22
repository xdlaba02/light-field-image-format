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
template<size_t D>
struct LfifEncoder {
  uint8_t color_depth; /**< @brief Number of bits per sample used by each decoded channel.*/

  std::array<uint64_t, D>     block_size;
  std::array<uint64_t, D + 1> img_dims; /**< @brief Dimensions of a encoded image + image count. The multiple of all values should be equal to number of pixels in encoded image.*/

  std::array<uint64_t, D>     img_dims_unaligned;
  std::array<uint64_t, D>     img_dims_aligned;

  std::array<uint64_t, D + 1> img_stride_unaligned;
  std::array<uint64_t, D + 1> img_stride_aligned;

  std::array<size_t, D>       block_dims; /**< @brief Dimensions of an encoded image in blocks. The multiple of all values should be equal to number of blocks in encoded image.*/
  std::array<size_t, D + 1>   block_stride;

  bool use_huffman; /**< @brief Huffman encoding will be used when this is true.*/
  bool use_prediction;

  QuantTable<D>      quant_table     [2];    /**< @brief Quantization matrices for luma and chroma.*/
  TraversalTable<D>  traversal_table [2];    /**< @brief Traversal matrices for luma and chroma.*/
  ReferenceBlock<D>  reference_block [2];    /**< @brief Reference blocks for luma and chroma to be used for traversal optimization.*/
  HuffmanWeights     huffman_weight  [2][2]; /**< @brief Weigth maps for luma and chroma and also for the DC and AC coefficients to be used for optimal Huffman encoding.*/
  HuffmanEncoder     huffman_encoder [2][2]; /**< @brief Huffman encoders for luma and chroma and also for the DC and AC coefficients.*/

  QuantTable<D>     *quant_tables    [3]; /**< @brief Pointer to quantization matrix used by each channel.*/
  ReferenceBlock<D> *reference_blocks[3]; /**< @brief Pointer to reference block used by each channel.*/
  TraversalTable<D> *traversal_tables[3]; /**< @brief Pointer to traversal matrix used by each channel.*/
  HuffmanWeights    *huffman_weights [3]; /**< @brief Pointer to weight maps used by each channel.*/
  HuffmanEncoder    *huffman_encoders[3]; /**< @brief Pointer to Huffman encoders used by each channel.*/

  size_t amp_bits;    /**< @brief Number of bits sufficient to contain maximum DCT coefficient.*/
  size_t zeroes_bits; /**< @brief Number of bits sufficient to contain run-length of zero coefficients.*/
  size_t class_bits;  /**< @brief Number of bits sufficient to contain number of bits of maximum DCT coefficient.*/
  size_t max_zeroes;  /**< @brief Maximum length of zero coefficient run-length.*/

  DynamicBlock<INPUTTRIPLET,  D> current_block;   /**< @brief Buffer for caching the block of pixels before encoding.*/
  DynamicBlock<INPUTUNIT,     D> input_block;     /**< @brief Buffer for caching the block of input samples.*/
  DynamicBlock<DCTDATAUNIT,   D> dct_block;       /**< @brief Buffer for caching the block of DCT coefficients.*/
  DynamicBlock<QDATAUNIT,     D> quantized_block; /**< @brief Buffer for caching the block of quantized coefficients.*/
  DynamicBlock<RunLengthPair, D> runlength;       /**< @brief Buffer for caching the block of run-length pairs.*/
};

/**
* @brief Function which (re)initializes the encoder structure.
* @param enc The encoder structure.
*/
template<size_t D>
void initEncoder(LfifEncoder<D> &enc) {
  enc.current_block.resize(enc.block_size);
  enc.input_block.resize(enc.block_size);
  enc.dct_block.resize(enc.block_size);
  enc.quantized_block.resize(enc.block_size);
  enc.runlength.resize(enc.block_size);

  for (size_t i {}; i < 2; i++) {
    enc.quant_table[i].resize(enc.block_size);
    enc.traversal_table[i].resize(enc.block_size);
    enc.reference_block[i].resize(enc.block_size);
  }

  enc.img_stride_unaligned[0] = 1;
  enc.img_stride_aligned[0]   = 1;
  enc.block_stride[0]         = 1;

  for (size_t i = 0; i < D; i++) {
    enc.block_dims[i]       = ceil(enc.img_dims[i] / static_cast<double>(enc.block_size[i]));
    enc.block_stride[i + 1] = enc.block_stride[i] * enc.block_dims[i];

    enc.img_dims_unaligned[i]       = enc.img_dims[i];
    enc.img_stride_unaligned[i + 1] = enc.img_stride_unaligned[i] * enc.img_dims_unaligned[i];

    enc.img_dims_aligned[i]       = ceil(enc.block_dims[i] * enc.block_size[i]);
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

  enc.amp_bits = ceil(log2(get_stride<D>(enc.block_size))) + enc.color_depth - D - (D/2);
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
template<size_t D>
int constructQuantizationTables(LfifEncoder<D> &enc, const std::string &table_type, float quality) {
  auto extend_table = [&](const QuantTable<2> &input) {
    QuantTable<D> output(8);

         if (table_type == "DCTDIAG")  averageDiagonalTable<2, D>(input, output);
    else if (table_type == "DCTCOPY")             copyTable<2, D>(input, output);
    else if (table_type == "FILLDIAG") averageDiagonalTable<2, D>(input, output);
    else if (table_type == "FILLCOPY")            copyTable<2, D>(input, output);

    return output;
  };

  auto scale_table = [&](const QuantTable<D> &input) {
    QuantTable<D> output(enc.block_size);
         if (table_type == "DCTDIAG")     scaleByDCT<D>(input, output);
    else if (table_type == "DCTCOPY")     scaleByDCT<D>(input, output);
    else if (table_type == "FILLDIAG") scaleFillNear<D>(input, output);
    else if (table_type == "FILLCOPY") scaleFillNear<D>(input, output);

    return output;
  };

  if (table_type == "DCTDIAG" || table_type == "FILLDIAG" || table_type == "DCTCOPY" || table_type == "FILLCOPY") {
    enc.quant_table[0] = scale_table(extend_table(baseLuma()));
    enc.quant_table[1] = scale_table(extend_table(baseChroma()));

    for (size_t i = 0; i < 2; i++) {
      applyQuality<D>(enc.quant_table[i], quality);
        clampTable<D>(enc.quant_table[i], 1, 255);
    }

    return 0;
  }
  else if (table_type == "DEFAULT" || table_type == "UNIFORM") {
    for (size_t i = 0; i < 2; i++) {
      uniformTable<D>(50, enc.quant_table[i]);
      applyQuality<D>(enc.quant_table[i], quality);
        clampTable<D>(enc.quant_table[i], 1, 255);
    }
    return 0;
  }

  return -1;
}

#include <iostream>
#include <iomanip>

/**
* @brief Function which performs arbitrary scan of an image. This function prepares a block into the encoding structure buffers.
* @param enc The encoder structure.
* @param input Callback function specified by client with signature T input(size_t index), where T is pixel/sample type.
* @param func Callback function which performs action on every block with signature void func(size_t channel), where channel is channel from which the extracted block is.
*/
template<size_t D, typename INPUTF, typename PERFF>
void performScan(LfifEncoder<D> &enc, INPUTF &&input, PERFF &&func) {
  auto outputF = [&](const std::array<size_t, D> &block_pos, const auto &value) {
    enc.current_block[block_pos] = value;
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
      getBlock<D>(enc.block_size.data(), inputF, pos_block, enc.img_dims_unaligned, outputF);

      for (size_t channel = 0; channel < 3; channel++) {
        for (size_t i = 0; i < enc.input_block.stride(D); i++) {
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
template<size_t D, typename F>
void referenceScan(LfifEncoder<D> &enc, F &&input) {
  auto perform = [&](size_t, size_t channel, const std::array<size_t, D> &) {
    forwardDiscreteCosineTransform<D>(enc.input_block,      enc.dct_block);
                          quantize<D>(enc.dct_block,        enc.quantized_block, *enc.quant_tables[channel]);
               addToReferenceBlock<D>(enc.quantized_block, *enc.reference_blocks[channel]);
  };

  performScan(enc, input, perform);
}

/**
* @brief Function which initializes linearization matrices.
* @param enc The encoder structure.
* @param table_type Type of the tables to be initialized.
* @return Nonzero if requested unknown table type.
*/
template<size_t D>
int constructTraversalTables(LfifEncoder<D> &enc, const std::string &table_type) {
  if (table_type == "REFERENCE") {
    for (size_t i = 0; i < 2; i++) {
      constructByReference(enc.reference_block[i], enc.traversal_table[i]);
    }
  }
  else if (table_type == "DEFAULT" || table_type == "ZIGZAG") {
    for (size_t i = 0; i < 2; i++) {
      constructZigzag(enc.traversal_table[i]);
    }
  }
  else if (table_type == "QTABLE") {
    for (size_t i = 0; i < 2; i++) {
      constructByQuantTable(enc.quant_table[i], enc.traversal_table[i]);
    }
  }
  else if (table_type == "RADIUS") {
    for (size_t i = 0; i < 2; i++) {
      constructByRadius(enc.traversal_table[i]);
    }
  }
  else if (table_type == "DIAGONALS") {
    for (size_t i = 0; i < 2; i++) {
      constructByDiagonals(enc.traversal_table[i]);
    }
  }
  else if (table_type == "HYPERBOLOID") {
    for (size_t i = 0; i < 2; i++) {
      constructByHyperboloid(enc.traversal_table[i]);
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
template<size_t D, typename F>
void huffmanScan(LfifEncoder<D> &enc, F &&input) {
  QDATAUNIT previous_DC [3] {};

  auto perform = [&](size_t, size_t channel, const std::array<size_t, D> &) {
    forwardDiscreteCosineTransform<D>(enc.input_block,      enc.dct_block);
                          quantize<D>(enc.dct_block,        enc.quantized_block,         *enc.quant_tables[channel]);
                      diffEncodeDC<D>(enc.quantized_block,  previous_DC[channel]);
                          traverse<D>(enc.quantized_block, *enc.traversal_tables[channel]);
                   runLengthEncode<D>(enc.quantized_block,  enc.runlength,                enc.max_zeroes);
                 huffmanAddWeights<D>(enc.runlength,        enc.huffman_weights[channel], enc.class_bits);
  };

  performScan(enc, input, perform);
}

/**
* @brief Function which initializes Huffman encoder from weights.
* @param enc The encoder structure.
*/
template<size_t D>
void constructHuffmanTables(LfifEncoder<D> &enc) {
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
template<size_t D>
void writeHeader(LfifEncoder<D> &enc, std::ostream &output) {
  output << "LFIF-" << D << "D\n";

  for (size_t i = 0; i < D; i++) {
    output << enc.block_size[i] << ' ';
  }
  output << "\n";

  writeValueToStream<uint8_t>(enc.color_depth, output);

  for (size_t i = 0; i < D + 1; i++) {
    writeValueToStream<uint64_t>(enc.img_dims[i], output);
  }

  for (size_t i = 0; i < 2; i++) {
    writeQuantToStream<D>(enc.quant_table[i], output);
  }

  writeValueToStream<uint8_t>(enc.use_huffman, output);
  writeValueToStream<uint8_t>(enc.use_prediction, output);


  if (enc.use_huffman) {
    for (size_t i = 0; i < 2; i++) {
      writeTraversalToStream(enc.traversal_table[i], output);
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
template<size_t D, typename F>
void outputScanHuffman_RUNLENGTH(LfifEncoder<D> &enc, F &&input, std::ostream &output) {
  QDATAUNIT previous_DC [3] {};
  OBitstream bitstream      {};

  bitstream.open(&output);

  auto perform = [&](size_t, size_t channel, const std::array<size_t, D> &) {
    forwardDiscreteCosineTransform<D>(enc.input_block,      enc.dct_block);
                          quantize<D>(enc.dct_block,        enc.quantized_block,          *enc.quant_tables[channel]);
                      diffEncodeDC<D>(enc.quantized_block,  previous_DC[channel]);
                          traverse<D>(enc.quantized_block, *enc.traversal_tables[channel]);
                   runLengthEncode<D>(enc.quantized_block,  enc.runlength,                 enc.max_zeroes);
           encodeHuffman_RUNLENGTH<D>(enc.runlength,        enc.huffman_encoders[channel], bitstream, enc.class_bits);
  };

  performScan(enc, input, perform);

  bitstream.flush();
}

#include <iostream>
#include <iomanip>

/**
* @brief Function which encodes the image to the stream with CABAC.
* @param enc The encoder structure.
* @param input Callback function specified by client with signature T input(size_t index), where T is pixel/sample type.
* @param output Output stream to which the image shall be encoded.
*/
template<size_t D, typename F>
void outputScanCABAC_DIAGONAL(LfifEncoder<D> &enc, F &&input, std::ostream &output) {
  std::vector<std::vector<size_t>>         scan_table(num_diagonals<D>(enc.block_size));

  std::array<CABACContextsDIAGONAL<D>,  2> contexts   { CABACContextsDIAGONAL<D>(enc.block_size), CABACContextsDIAGONAL<D>(enc.block_size) };
  OBitstream                               bitstream  {};
  CABACEncoder                             cabac      {};
  size_t                                   threshold  {};

  std::array<DynamicBlock<INPUTUNIT, D>, 3> decoded          {};
  DynamicBlock<INPUTUNIT, D>                prediction_block {};

  if (enc.use_prediction) {
    decoded[0].resize(enc.img_dims_aligned);
    decoded[1].resize(enc.img_dims_aligned);
    decoded[2].resize(enc.img_dims_aligned);
    prediction_block.resize(enc.block_size);
  }

  threshold = num_diagonals<D>(enc.block_size) / 2;

  iterate_dimensions<D>(enc.block_size, [&](const auto &pos) {
    size_t diagonal {};
    for (size_t i = 0; i < D; i++) {
      diagonal += pos[i];
    }

    scan_table[diagonal].push_back(make_index(enc.block_size, pos));
  });

  bitstream.open(&output);
  cabac.init(bitstream);

  uint64_t prediction_type {};

  auto inputF = [&](const std::array<size_t, D> &pos) -> const auto & {
    return enc.input_block[pos];
  };

  auto perform = [&](size_t, size_t channel, const std::array<size_t, D> &block) {
    auto outputF = [&](const std::array<size_t, D> &pos, const auto &value) {
      decoded[channel][pos] = value;
    };

    bool previous_block_available[D] {};
    for (size_t i { 0 }; i < D; i++) {
      if (block[i]) {
        previous_block_available[i] = true;
      }
      else {
        previous_block_available[i] = false;
      }
    }

    auto predInputF = [&](std::array<int64_t, D> &block_pos) {
      for (size_t i { 1 }; i < D; i++) {
        if (block_pos[i] >= static_cast<int64_t>(enc.block_size[i])) {
          block_pos[i] = enc.block_size[i] - 1;
        }
      }

      for (size_t i { 0 }; i < D; i++) {
        if (!previous_block_available[i] && block_pos[i] < 0) {
          block_pos[i] = 0;
        }
      }

      int64_t max_pos {};
      for (size_t i = 0; i < D; i++) {
        if (previous_block_available[i] && (block_pos[i] + 1) > max_pos) {
          max_pos = block_pos[i] + 1;
        }
      }

      int64_t min_pos { max_pos }; // asi bude lepsi int64t maximum

      for (size_t i = 0; i < D; i++) {
        if (previous_block_available[i] && (block_pos[i] + 1) < min_pos) {
          min_pos = block_pos[i] + 1;
        }
      }

      for (size_t i = 0; i < D; i++) {
        if (previous_block_available[i]) {
          block_pos[i] -= min_pos;
        }
      }

      std::array<size_t, D> img_pos {};
      for (size_t i { 0 }; i < D; i++) {
        img_pos[i] = block[i] * enc.block_size[i] + block_pos[i];

        if (img_pos[i] >= enc.img_dims_aligned[i]) {
          img_pos[i] = enc.img_dims_aligned[i] - 1;
        }
      }

      return decoded[channel][img_pos];
    };

    if (enc.use_prediction) {
      if (channel == 0) {
        prediction_type = find_best_prediction_type<D>(enc.input_block, predInputF);
        encodePredictionType<D>(prediction_type, cabac, contexts[0]);

        for (size_t d = 0; d < D; d++) {
          std::cerr <<  std::setw(3) << std::fixed << block[d];
        }
        std::cerr << ": ";

        if (prediction_type == 0) {
          std::cerr << "NO PREDICTION\n";
        }
        else if (prediction_type == 1) {
          std::cerr << "DC PREDICTION\n";
        }
        else if (prediction_type == 2) {
          std::cerr << "PLANAR PREDICTION\n";
        }
        else if (prediction_type >= 3) {
          size_t dir = prediction_type - 3;
          int8_t direction[D] {};

          std::cerr << "[";
          for (size_t d { 0 }; d < D; d++) {
            direction[d] = dir % constpow(5, d + 1) / constpow(5, d);
            direction[d] -= 2;
            std::cerr << std::setw(3) << std::fixed << (int)direction[d];
          }
          std::cerr << " ]\n";
        }
      }

                             predict<D>(prediction_block,      prediction_type,      predInputF                                           );
                     applyPrediction<D>(enc.input_block,       prediction_block                                                           );
    }

      forwardDiscreteCosineTransform<D>(enc.input_block,       enc.dct_block                                                              );
                            quantize<D>(enc.dct_block,         enc.quantized_block, *enc.quant_tables[channel]                            );

    if (enc.use_prediction) {
                          dequantize<D>(enc.quantized_block,   enc.dct_block,       *enc.quant_tables[channel]                            );
      inverseDiscreteCosineTransform<D>(enc.dct_block,         enc.input_block                                                            );
                    disusePrediction<D>(enc.input_block,       prediction_block                                                           );
                            putBlock<D>(enc.block_size.data(), inputF,               block,                  enc.img_dims_aligned, outputF);
    }
    
                encodeCABAC_DIAGONAL<D>(enc.quantized_block,   cabac,                contexts[channel != 0], threshold, scan_table        );
  };

  performScan(enc, input, perform);

  cabac.terminate();
  bitstream.flush();
}

template<size_t D, typename F>
void outputScanCABAC_RUNLENGTH(LfifEncoder<D> &enc, F &&input, std::ostream &output) {
  std::array<CABACContextsRUNLENGTH<D>, 3> contexts    {};
  std::array<QDATAUNIT,                 3> previous_DC {};
  OBitstream                               bitstream   {};
  CABACEncoder                             cabac       {};

  bitstream.open(&output);
  cabac.init(bitstream);

  auto perform = [&](size_t, size_t channel, const std::array<size_t, D> &) {
    forwardDiscreteCosineTransform<D>(enc.input_block,      enc.dct_block);
                          quantize<D>(enc.dct_block,        enc.quantized_block, *enc.quant_tables[channel]);
                      diffEncodeDC<D>(enc.quantized_block,  previous_DC[channel]);
                          traverse<D>(enc.quantized_block, *enc.traversal_tables[channel]);
                   runLengthEncode<D>(enc.quantized_block,  enc.runlength,        enc.max_zeroes);
             encodeCABAC_RUNLENGTH<D>(enc.runlength,        cabac,                contexts[channel], enc.class_bits);
  };

  performScan(enc, input, perform);

  cabac.terminate();
  bitstream.flush();
}

template<size_t D, typename F>
void outputScanCABAC_JPEG(LfifEncoder<D> &enc, F &&input, std::ostream &output) {
  std::array<CABACContextsJPEG<D>, 3> contexts    { enc.amp_bits, enc.amp_bits, enc.amp_bits };
  std::array<QDATAUNIT,            3> previous_DC {};
  std::array<QDATAUNIT,            3> previous_DC_diff {};
  OBitstream                          bitstream   {};
  CABACEncoder                        cabac       {};

  bitstream.open(&output);
  cabac.init(bitstream);

  auto perform = [&](size_t, size_t channel, const std::array<size_t, D> &) {
    forwardDiscreteCosineTransform<D>(enc.input_block,      enc.dct_block);
                          quantize<D>(enc.dct_block,        enc.quantized_block, *enc.quant_tables[channel]);
                      diffEncodeDC<D>(enc.quantized_block,  previous_DC[channel]);
                          traverse<D>(enc.quantized_block, *enc.traversal_tables[channel]);
                  encodeCABAC_JPEG<D>(enc.quantized_block,  cabac,                contexts[channel], previous_DC_diff[channel], enc.amp_bits);
  };

  performScan(enc, input, perform);

  cabac.terminate();
  bitstream.flush();
}

template<size_t D, typename F>
void outputScanCABAC_H264(LfifEncoder<D> &enc, F &&input, std::ostream &output) {
  std::array<CABACContextsH264<D>, 3> contexts    {};
  std::array<QDATAUNIT,            3> previous_DC {};
  OBitstream                          bitstream   {};
  CABACEncoder                        cabac       {};

  bitstream.open(&output);
  cabac.init(bitstream);

  auto perform = [&](size_t, size_t channel, const std::array<size_t, D> &) {
    forwardDiscreteCosineTransform<D>(enc.input_block,      enc.dct_block);
                          quantize<D>(enc.dct_block,        enc.quantized_block, *enc.quant_tables[channel]);
                      diffEncodeDC<D>(enc.quantized_block,  previous_DC[channel]);
                          traverse<D>(enc.quantized_block, *enc.traversal_tables[channel]);
                  encodeCABAC_H264<D>(enc.quantized_block,  cabac,                contexts[channel]);
  };

  performScan(enc, input, perform);

  cabac.terminate();
  bitstream.flush();
}

#endif
