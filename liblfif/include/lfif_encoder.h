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

template<size_t BS, size_t D>
struct lfifCompress {
  template<typename F>
  lfifCompress(F &&input, const uint64_t img_dims[D+1], uint8_t quality, uint16_t max_rgb_value, std::ostream &output) {
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

    size_t rgb_bits    {};
    size_t amp_bits    {};
    size_t zeroes_bits {};
    size_t class_bits  {};
    size_t max_zeroes  {};

    Block<std::array<YCBCRUNIT, 3>, BS, D> current_block   {};
    Block<YCBCRUNIT,                BS, D> ycbcr_block     {};
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

    rgb_bits = ceil(log2(max_rgb_value));
    amp_bits = ceil(log2(constpow(BS, D))) + rgb_bits - D - (D/2);
    class_bits = RunLengthPair::classBits(amp_bits);
    zeroes_bits = RunLengthPair::zeroesBits(class_bits);
    max_zeroes = constpow(2, zeroes_bits);

    quant_table[0]
    . baseDiagonalTable(QuantTable<BS, D>::base_luma, quality);

    quant_table[1]
    . baseDiagonalTable(QuantTable<BS, D>::base_chroma, quality);


    for (size_t img = 0; img < img_dims[D]; img++) {

      auto inputF = [&](size_t index) {
        return input(img * pixels_cnt + index);
      };

      for (size_t block = 0; block < blocks_cnt; block++) {

        getBlock<BS, D>(inputF, block, img_dims, outputF);

        for (size_t channel = 0; channel < 3; channel++) {
          for (size_t i = 0; i < constpow(BS, D); i++) {
            ycbcr_block[i] = current_block[i][channel];
          }

          forwardDiscreteCosineTransform<BS, D>(ycbcr_block, dct_block);
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
            ycbcr_block[i] = current_block[i][channel];
          }

          forwardDiscreteCosineTransform<BS, D>(ycbcr_block, dct_block);
          quantize<BS, D>(dct_block, quantized_block, *quant_tables[channel]);
          diffEncodeDC<BS, D>(quantized_block, previous_DC[channel]);
          traverse<BS, D>(quantized_block, *traversal_tables[channel]);
          runLengthEncode<BS, D>(quantized_block, runlength, max_zeroes);
          huffmanAddWeights<BS, D>(runlength, huffman_weights[channel], class_bits);
        }
      }
    }

    for (size_t i = 0; i < 2; i++) {
      quant_table[i]
      . writeToStream(output);
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
            ycbcr_block[i] = current_block[i][channel];
          }

          forwardDiscreteCosineTransform<BS, D>(ycbcr_block, dct_block);
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
