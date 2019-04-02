/******************************************************************************\
* SOUBOR: lfif_decoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_DECODER_H
#define LFIF_DECODER_H

#include "block_decompress_chain.h"
#include "colorspace.h"

#include <cstdint>
#include <istream>

template<size_t BS, size_t D>
struct lfifDecompress {
  template <typename F>
  lfifDecompress(std::istream &input, const uint64_t img_dims[D+1], uint16_t max_rgb_value, F &&output) {
    QuantTable<BS, D>       quant_table         [2]    {};
    TraversalTable<BS, D>   traversal_table     [2]    {};
    HuffmanDecoder          huffman_decoder     [2][2] {};

    QDATAUNIT               previous_DC         [3]    {};
    HuffmanDecoder         *huffman_decoders_ptr[3]    {};
    TraversalTable<BS, D>  *traversal_table_ptr [3]    {};
    QuantTable<BS, D>      *quant_table_ptr     [3]    {};

    size_t blocks_cnt {};
    size_t pixels_cnt {};

    size_t rgb_bits   {};
    size_t amp_bits   {};
    size_t class_bits {};

    IBitstream bitstream {};

    Block<std::array<INPUTUNIT, 3>, BS, D> current_block   {};
    Block<           RunLengthPair, BS, D> runlength       {};
    Block<               QDATAUNIT, BS, D> quantized_block {};
    Block<             DCTDATAUNIT, BS, D> dct_block       {};
    Block<               INPUTUNIT, BS, D> ycbcr_block     {};

    auto inputF = [&](size_t index) -> const auto & {
      return current_block[index];
    };

    huffman_decoders_ptr[0] =  huffman_decoder[0];
    huffman_decoders_ptr[1] =  huffman_decoder[1];
    huffman_decoders_ptr[2] =  huffman_decoder[1];

    traversal_table_ptr[0] = &traversal_table[0];
    traversal_table_ptr[1] = &traversal_table[1];
    traversal_table_ptr[2] = &traversal_table[1];

    quant_table_ptr[0]     = &quant_table[0];
    quant_table_ptr[1]     = &quant_table[1];
    quant_table_ptr[2]     = &quant_table[1];

    for (size_t i = 0; i < 2; i++) {
      quant_table[i]
      . readFromStream(input);
    }

    for (size_t i = 0; i < 2; i++) {
      traversal_table[i]
      . readFromStream(input);
    }

    for (size_t y = 0; y < 2; y++) {
      for (size_t x = 0; x < 2; x++) {
        huffman_decoder[y][x]
        . readFromStream(input);
      }
    }

    blocks_cnt = 1;
    pixels_cnt = 1;

    for (size_t i = 0; i < D; i++) {
      blocks_cnt *= ceil(img_dims[i]/static_cast<double>(BS));
      pixels_cnt *= img_dims[i];
    }

    rgb_bits = ceil(log2(max_rgb_value));
    amp_bits = ceil(log2(constpow(BS, D))) + rgb_bits - D - (D/2);
    class_bits = RunLengthPair::classBits(amp_bits);

    bitstream.open(&input);

    previous_DC[0] = 0;
    previous_DC[1] = 0;
    previous_DC[2] = 0;

    for (size_t img = 0; img < img_dims[D]; img++) {

      auto outputF = [&](size_t index, const auto &value) {
        output(img * pixels_cnt + index, value);
      };

      for (size_t block = 0; block < blocks_cnt; block++) {
        for (size_t channel = 0; channel < 3; channel++) {
          decodeFromStream<BS, D>(bitstream, runlength, huffman_decoders_ptr[channel], class_bits);
          runLengthDecode<BS, D>(runlength, quantized_block);
          detraverse<BS, D>(quantized_block, *traversal_table_ptr[channel]);
          diffDecodeDC<BS, D>(quantized_block, previous_DC[channel]);
          dequantize<BS, D>(quantized_block, dct_block, *quant_table_ptr[channel]);
          inverseDiscreteCosineTransform<BS, D>(dct_block, ycbcr_block);

          for (size_t i = 0; i < constpow(BS, D); i++) {
            current_block[i][channel] = ycbcr_block[i];
          }
        }

        putBlock<BS, D>(inputF, block, img_dims, outputF);
      }
    }
  }
};

#endif
