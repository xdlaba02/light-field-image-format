/******************************************************************************\
* SOUBOR: block_decompress_chain.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef BLOCK_DECOMPRESS_CHAIN
#define BLOCK_DECOMPRESS_CHAIN

#include "lfiftypes.h"
#include "huffman.h"
#include "runlength.h"
#include "traversal_table.h"
#include "quant_table.h"
#include "block.h"
#include "dct.h"

#include <vector>

template <size_t BS, size_t D>
void decodeFromStream(IBitstream &bitstream, Block<RunLengthPair, BS, D> &runlength, const HuffmanDecoder huffman_decoders[2], size_t class_bits) {
  auto pairs_it = std::begin(runlength);

  pairs_it->huffmanDecodeFromStream(huffman_decoders[0], bitstream, class_bits);

  do {
    pairs_it++;
    pairs_it->huffmanDecodeFromStream(huffman_decoders[1], bitstream, class_bits);
  } while(!pairs_it->eob() && (pairs_it != (std::end(runlength) - 1)));
}

template <size_t BS, size_t D>
void runLengthDecode(const Block<RunLengthPair, BS, D> &runlength, Block<QDATAUNIT, BS, D> &traversed_block) {
  traversed_block.fill(0);

  size_t pixel_index = 0;
  auto pairs_it = std::begin(runlength);
  do {
    pixel_index += pairs_it->zeroes;
    traversed_block[pixel_index] = pairs_it->amplitude;
    pixel_index++;
    pairs_it++;
  } while (!pairs_it->eob() && (pairs_it != std::end(runlength)));
}

template <size_t BS, size_t D>
void detraverse(Block<QDATAUNIT, BS, D> &traversed_block, const TraversalTable<BS, D> &traversal_table) {
  Block<QDATAUNIT, BS, D> traversed_block_copy(traversed_block);

  for (size_t i = 0; i < constpow(BS, D); i++) {
    traversed_block[i] = traversed_block_copy[traversal_table[i]];
  }
}

template <size_t BS, size_t D>
void diffDecodeDC(Block<QDATAUNIT, BS, D> &diff_encoded_block, QDATAUNIT &previous_DC) {
  diff_encoded_block[0] += previous_DC;
  previous_DC = diff_encoded_block[0];
}

template <size_t BS, size_t D>
void dequantize(const Block<QDATAUNIT, BS, D> &quantized_block, Block<DCTDATAUNIT, BS, D> &dct_block, const QuantTable<BS, D> &quant_table) {
  for (size_t i = 0; i < constpow(BS, D); i++) {
    dct_block[i] = static_cast<DCTDATAUNIT>(quantized_block[i]) * quant_table[i];
  }
}

template <size_t BS, size_t D>
void inverseDiscreteCosineTransform(const Block<DCTDATAUNIT, BS, D> &dct_block, Block<INPUTUNIT, BS, D> &output_block) {
  output_block.fill(0);

  auto inputF = [&](size_t index) {
    return dct_block[index];
  };

  auto outputF = [&](size_t index) -> auto & {
    return output_block[index];
  };

  idct<BS, D>(inputF, outputF);
}

#endif
