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

template<size_t D>
struct lfif_decompress {
  template <typename F>
  lfif_decompress(std::istream &input, const uint64_t img_dims[D+1], uint16_t max_rgb_value, F &&output) {
    BlockDecompressChain<D> block_decompress_chain {};
    QuantTable<D>           quant_table     [2]    {};
    TraversalTable<D>       traversal_table [2]    {};
    HuffmanDecoder          huffman_decoder [2][2] {};

    QDATAUNIT               previous_DC     [3]    {};

    HuffmanDecoder         *huffman_decoders[3]    {};
    TraversalTable<D>      *traversal_tables[3]    {};
    QuantTable<D>          *quant_tables    [3]    {};

    size_t blocks_cnt {};
    size_t pixels_cnt {};

    size_t rgb_bits   {};
    size_t amp_bits   {};
    size_t class_bits {};

    huffman_decoders[0] =  huffman_decoder[0];
    huffman_decoders[1] =  huffman_decoder[1];
    huffman_decoders[2] =  huffman_decoder[1];

    traversal_tables[0] = &traversal_table[0];
    traversal_tables[1] = &traversal_table[1];
    traversal_tables[2] = &traversal_table[1];

    quant_tables[0]     = &quant_table[0];
    quant_tables[1]     = &quant_table[1];
    quant_tables[2]     = &quant_table[1];

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
      blocks_cnt *= ceil(img_dims[i]/static_cast<double>(BLOCK_SIZE));
      pixels_cnt *= img_dims[i];
    }

    rgb_bits = ceil(log2(max_rgb_value));
    amp_bits = ceil(log2(constpow(BLOCK_SIZE, D))) + rgb_bits - D - (D/2);
    class_bits = RunLengthPair::classBits(amp_bits);

    previous_DC[0] = 0;
    previous_DC[1] = 0;
    previous_DC[2] = 0;

    IBitstream bitstream(&input);

    for (size_t img = 0; img < img_dims[D]; img++) {
      for (size_t block = 0; block < blocks_cnt; block++) {
        for (size_t channel = 0; channel < 3; channel++) {
          block_decompress_chain
          . decodeFromStream(huffman_decoders[channel], bitstream, class_bits)
          . runLengthDecode()
          . detraverse(*traversal_tables[channel])
          . diffDecodeDC(previous_DC[channel])
          . dequantize(*quant_tables[channel])
          . inverseDiscreteCosineTransform();

          auto inputF = [&](size_t index) {
            return block_decompress_chain.m_ycbcr_block[index];
          };

          auto outputF = [&](size_t index) -> auto & {
            return output(channel, img * pixels_cnt + index);
          };

          putBlock<D>(inputF, block, img_dims, outputF);
        }
      }
    }
  }
};

#endif
