/**
* @file lfif_decoder.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for decoding an image.
*/

#ifndef LFIF_DECODER_H
#define LFIF_DECODER_H

#include "block_compress_chain.h"
#include "block_decompress_chain.h"
#include "bitstream.h"

#include <cstdint>
#include <istream>
#include <sstream>

/**
* @brief Base structure for decoding an image.
*/
template<size_t D>
struct LfifDecoder {
  uint8_t color_depth;    /**< @brief Number of bits per sample used by each decoded channel.*/

  std::array<uint64_t, D>     block_size;
  std::array<uint64_t, D + 1> img_dims; /**< @brief Dimensions of a decoded image + image count.*/

  std::array<uint64_t, D> img_dims_unaligned;
  std::array<uint64_t, D> img_dims_aligned;

  std::array<uint64_t, D + 1> img_stride_unaligned;
  std::array<uint64_t, D + 1> img_stride_aligned;

  std::array<size_t, D>     block_dims; /**< @brief Dimensions of an encoded image in blocks. The multiple of all values should be equal to number of blocks in encoded image.*/
  std::array<size_t, D + 1> block_stride;

  bool use_huffman; /**< @brief Huffman Encoding was used when this is true.*/
  bool use_prediction;

  QuantTable<D>      quant_table         [2];    /**< @brief Quantization matrices for luma and chroma.*/
  TraversalTable<D>  traversal_table     [2];    /**< @brief Traversal matrices for luma and chroma.*/
  HuffmanDecoder     huffman_decoder     [2][2]; /**< @brief Huffman decoders for luma and chroma and also for the DC and AC coefficients.*/

  HuffmanDecoder    *huffman_decoders_ptr[3]; /**< @brief Pointer to Huffman decoders used by each channel.*/
  TraversalTable<D> *traversal_table_ptr [3]; /**< @brief Pointer to traversal matrix used by each channel.*/
  QuantTable<D>     *quant_table_ptr     [3]; /**< @brief Pointer to quantization matrix used by each channel.*/

  size_t amp_bits;   /**< @brief Number of bits sufficient to contain maximum DCT coefficient.*/
  size_t class_bits; /**< @brief Number of bits sufficient to contain number of bits of maximum DCT coefficient.*/

  DynamicBlock<INPUTTRIPLET,  D> current_block;   /**< @brief Buffer for caching the block of pixels before returning to client.*/
  DynamicBlock<RunLengthPair, D> runlength;       /**< @brief Buffer for caching the block of run-length pairs when decoding.*/
  DynamicBlock<QDATAUNIT,     D> quantized_block; /**< @brief Buffer for caching the block of quantized coefficients.*/
  DynamicBlock<DCTDATAUNIT,   D> dct_block;       /**< @brief Buffer for caching the block of DCT coefficients.*/
  DynamicBlock<INPUTUNIT,     D> output_block;    /**< @brief Buffer for caching the block of samples.*/
};

/**
* @brief Function which reads the image header from stream.
* @param dec The decoder structure.
* @param input The input stream.
* @return Zero if the header was succesfully read, nonzero else.
*/
template<size_t D>
int readHeader(LfifDecoder<D> &dec, std::istream &input) {
  std::string       magic_number     {};
  std::stringstream magic_number_cmp {};

  magic_number_cmp << "LFIF-" << D << "D";

  input >> magic_number;
  input.ignore();

  if (magic_number != magic_number_cmp.str()) {
    return -1;
  }

  std::string block_size {};
  for (size_t i = 0; i < D; i++) {
    input >> dec.block_size[i];
    input.ignore();
  }
  input.ignore();

  dec.color_depth = readValueFromStream<uint8_t>(input);

  for (size_t i = 0; i < D + 1; i++) {
    dec.img_dims[i] = readValueFromStream<uint64_t>(input);
  }

  for (size_t i = 0; i < 2; i++) {
    dec.quant_table[i] = readQuantFromStream<D>(dec.block_size, input);
  }

  dec.use_huffman    = readValueFromStream<uint8_t>(input);
  dec.use_prediction = readValueFromStream<uint8_t>(input);

  if (dec.use_huffman) {
    for (size_t i = 0; i < 2; i++) {
      dec.traversal_table[i] = readTraversalFromStream(dec.block_size, input);
    }

    for (size_t y = 0; y < 2; y++) {
      for (size_t x = 0; x < 2; x++) {
        dec.huffman_decoder[y][x]
        . readFromStream(input);
      }
    }
  }

  return 0;
}

/**
* @brief Function which (re)initializes the decoder structure.
* @param dec The decoder structure.
*/
template<size_t D>
void initDecoder(LfifDecoder<D> &dec) {
  dec.current_block.resize(dec.block_size);
  dec.output_block.resize(dec.block_size);
  dec.dct_block.resize(dec.block_size);
  dec.quantized_block.resize(dec.block_size);
  dec.runlength.resize(dec.block_size);

  dec.img_stride_unaligned[0] = 1;
  dec.img_stride_aligned[0]   = 1;
  dec.block_stride[0]         = 1;

  for (size_t i = 0; i < D; i++) {
    dec.block_dims[i]       = ceil(dec.img_dims[i] / static_cast<double>(dec.block_size[i]));
    dec.block_stride[i + 1] = dec.block_stride[i] * dec.block_dims[i];

    dec.img_dims_unaligned[i]       = dec.img_dims[i];
    dec.img_stride_unaligned[i + 1] = dec.img_stride_unaligned[i] * dec.img_dims_unaligned[i];

    dec.img_dims_aligned[i]       = ceil(dec.block_dims[i] * dec.block_size[i]);
    dec.img_stride_aligned[i + 1] = dec.img_stride_aligned[i] * dec.img_dims_aligned[i];
  }

  dec.huffman_decoders_ptr[0] = dec.huffman_decoder[0];
  dec.huffman_decoders_ptr[1] = dec.huffman_decoder[1];
  dec.huffman_decoders_ptr[2] = dec.huffman_decoder[1];

  dec.traversal_table_ptr[0]  = &dec.traversal_table[0];
  dec.traversal_table_ptr[1]  = &dec.traversal_table[1];
  dec.traversal_table_ptr[2]  = &dec.traversal_table[1];

  dec.quant_table_ptr[0]      = &dec.quant_table[0];
  dec.quant_table_ptr[1]      = &dec.quant_table[1];
  dec.quant_table_ptr[2]      = &dec.quant_table[1];

  dec.amp_bits = ceil(log2(get_stride<D>(dec.block_size))) + dec.color_depth - D - (D/2);
  dec.class_bits = RunLengthPair::classBits(dec.amp_bits);
}

/**
* @brief Function which decodes Huffman encoded image from stream.
* @param dec The decoder structure.
* @param input Input stream from which the image will be decoded.
* @param output Output callback function which will be returning pixels with signature void output(size_t index, INPUTTRIPLET value), where index is a pixel index in memory and value is said pixel.
*/
template<size_t D, typename F>
void decodeScanHuffman(LfifDecoder<D> &dec, std::istream &input, F &&output) {
  IBitstream bitstream       {};
  QDATAUNIT  previous_DC [3] {};

  bitstream.open(&input);

  auto inputF = [&](const std::array<size_t, D> &pos) -> const auto & {
    return dec.current_block[pos];
  };

  for (size_t img = 0; img < dec.img_dims[D]; img++) {
    iterate_dimensions<D>(dec.block_dims, [&](const std::array<size_t, D> &block) {
      for (size_t channel = 0; channel < 3; channel++) {
               decodeHuffman_RUNLENGTH<D>(bitstream,            dec.runlength, dec.huffman_decoders_ptr[channel], dec.class_bits);
                       runLengthDecode<D>(dec.runlength,        dec.quantized_block);
                            detraverse<D>(dec.quantized_block, *dec.traversal_table_ptr[channel]);
                          diffDecodeDC<D>(dec.quantized_block,  previous_DC[channel]);
                            dequantize<D>(dec.quantized_block,  dec.dct_block, *dec.quant_table_ptr[channel]);
        inverseDiscreteCosineTransform<D>(dec.dct_block,        dec.output_block);

        for (size_t i = 0; i < get_stride<D>(dec.block_size); i++) {
          dec.current_block[i][channel] = dec.output_block[i];
        }
      }

      auto outputF = [&](const std::array<size_t, D> &pos, const auto &value) {
        size_t img_index {};

        for (size_t i { 0 }; i < D; i++) {
          img_index += pos[i] * dec.img_stride_unaligned[i];
        }
        img_index += img * dec.img_stride_unaligned[D];

        output(img_index, value);
      };

      putBlock<D>(dec.block_size.data(), inputF, block, dec.img_dims_unaligned, outputF);
    });
  }
}

/**
* @brief Function which decodes CABAC encoded image from stream.
* @param dec The decoder structure.
* @param input Input stream from which the image will be decoded.
* @param output Output callback function which will be returning pixels with signature void output(size_t index, INPUTTRIPLET value), where index is a pixel index in memory and value is said pixel.
*/
template<size_t D, typename F>
void decodeScanCABAC(LfifDecoder<D> &dec, std::istream &input, F &&output) {
  std::vector<std::vector<size_t>>        scan_table(num_diagonals<D>(dec.block_size));
  std::array<CABACContextsDIAGONAL<D>, 2> contexts         {CABACContextsDIAGONAL<D>(dec.block_size), CABACContextsDIAGONAL<D>(dec.block_size)};
  IBitstream                              bitstream        {};
  CABACDecoder                            cabac            {};
  size_t                                  threshold        {};

  std::array<DynamicBlock<INPUTUNIT, D>, 3> decoded          {};
  DynamicBlock<INPUTUNIT, D>                prediction_block {};

  if (dec.use_prediction) {
    decoded[0].resize(dec.img_dims_aligned);
    decoded[1].resize(dec.img_dims_aligned);
    decoded[2].resize(dec.img_dims_aligned);
    prediction_block.resize(dec.block_size);
  }

  threshold = num_diagonals<D>(dec.block_size) / 2;

  iterate_dimensions<D>(dec.block_size, [&](const auto &pos) {
    size_t diagonal {};
    for (size_t i = 0; i < D; i++) {
      diagonal += pos[i];
    }

    scan_table[diagonal].push_back(make_index(dec.block_size, pos));
  });

  bitstream.open(&input);
  cabac.init(bitstream);

  auto inputF = [&](const std::array<size_t, D> &pos) -> const auto & {
    return dec.current_block[pos];
  };

  auto inputFP = [&](const std::array<size_t, D> &pos) -> const auto & {
    return dec.output_block[pos];
  };

  for (size_t img = 0; img < dec.img_dims[D]; img++) {
    auto outputF = [&](const std::array<size_t, D> &img_pos, const auto &value) {
      size_t img_index {};

      for (size_t i { 0 }; i < D; i++) {
        img_index += img_pos[i] * dec.img_stride_unaligned[i];
      }

      img_index += img * dec.img_stride_unaligned[D];

      output(img_index, value);
    };

    iterate_dimensions<D>(dec.block_dims, [&](const std::array<size_t, D> &block) {
      bool previous_block_available[D] {};
      for (size_t i { 0 }; i < D; i++) {
        if (block[i]) {
          previous_block_available[i] = true;
        }
        else {
          previous_block_available[i] = false;
        }
      }

      uint64_t prediction_type {};
      for (size_t channel = 0; channel < 3; channel++) {

        auto predInputF = [&](std::array<int64_t, D> &block_pos) {
          for (size_t i { 1 }; i < D; i++) {
            if (block_pos[i] >= static_cast<int64_t>(dec.block_size[i])) {
              block_pos[i] = dec.block_size[i] - 1;
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
            img_pos[i] = block[i] * dec.block_size[i] + block_pos[i];

            if (img_pos[i] >= dec.img_dims_aligned[i]) {
              img_pos[i] = dec.img_dims_aligned[i] - 1;
            }
          }

          return decoded[channel][img_pos];
        };

        auto outputFP = [&](const std::array<size_t, D> &img_pos, const auto &value) {
          decoded[channel][img_pos] = value;
        };

        if (dec.use_prediction) {
          if (channel == 0) {
            decodePredictionType<D>(prediction_type, cabac, contexts[0]);
          }
                               predict<D>(prediction_block,      prediction_type,                predInputF                    );
        }
                  decodeCABAC_DIAGONAL<D>(dec.quantized_block,   cabac, contexts[channel != 0],  threshold, scan_table         );
                            dequantize<D>(dec.quantized_block,   dec.dct_block,                 *dec.quant_table_ptr[channel]  );
        inverseDiscreteCosineTransform<D>(dec.dct_block,         dec.output_block                                              );

        if (dec.use_prediction) {
                      disusePrediction<D>(dec.output_block,      prediction_block                                              );
                              putBlock<D>(dec.block_size.data(), inputFP,             block,                          dec.img_dims_aligned, outputFP);
        }

        for (size_t i = 0; i < get_stride<D>(dec.block_size); i++) {
          dec.current_block[i][channel] = dec.output_block[i];
        }
      }

      putBlock<D>(dec.block_size.data(), inputF, block, dec.img_dims_unaligned, outputF);
    });
  }

  cabac.terminate();
}
#endif
