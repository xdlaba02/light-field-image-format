/******************************************************************************\
* SOUBOR: block_compress_chain.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef BLOCK_COMPRESS_CHAIN
#define BLOCK_COMPRESS_CHAIN

#include "lfiftypes.h"
#include "quant_table.h"
#include "traversal_table.h"
#include "huffman.h"
#include "dct.h"
#include "block.h"
#include "runlength.h"

#include <cstdlib>
#include <cstdint>

template <size_t D>
void forwardDiscreteCosineTransform(const Block<YCBCRUNIT, D> &ycbcr_block, Block<DCTDATAUNIT, D> &transformed_block) {
  transformed_block.fill(0);

  auto inputF = [&](size_t index) {
    return ycbcr_block[index];
  };

  auto outputF = [&](size_t index) -> auto & {
    return transformed_block[index];
  };

  fdct<D>(inputF, outputF);
}

template <size_t D>
void quantize(const Block<DCTDATAUNIT, D> &transformed_block, Block<QDATAUNIT, D> &quantized_block, const QuantTable<D> &quant_table) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    quantized_block[i] = transformed_block[i] / quant_table[i];
  }
}


template <size_t D>
void addToReferenceBlock(const Block<QDATAUNIT, D> &quantized_block, ReferenceBlock<D> &reference) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    reference[i] += abs(quantized_block[i]);
  }
}

template <size_t D>
void diffEncodeDC(Block<QDATAUNIT, D> &quantized_block, QDATAUNIT &previous_DC) {
  QDATAUNIT current_DC = quantized_block[0];
  quantized_block[0] -= previous_DC;
  previous_DC = current_DC;
}

template <size_t D>
void traverse(Block<QDATAUNIT, D> &diff_encoded_block, const TraversalTable<D> &traversal_table) {
  Block<QDATAUNIT, D> diff_encoded_copy(diff_encoded_block);

  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    diff_encoded_block[traversal_table[i]] = diff_encoded_copy[i];
  }
}

template <size_t D>
void runLengthEncode(const Block<QDATAUNIT, D> &traversed_block, Block<RunLengthPair, D> &runlength, size_t max_zeroes) {
  auto pairs_it = std::begin(runlength);

  auto push_pair = [&](RunLengthPair &&pair) {
    *pairs_it = pair;
    pairs_it++;
  };

  push_pair({0, traversed_block[0]});

  size_t zeroes = 0;
  for (size_t i = 1; i < constpow(BLOCK_SIZE, D); i++) {
    if (traversed_block[i] == 0) {
      zeroes++;
    }
    else {
      while (zeroes >= max_zeroes) {
        push_pair({max_zeroes - 1, 0});
        zeroes -= max_zeroes;
      }
      push_pair({zeroes, traversed_block[i]});
      zeroes = 0;
    }
  }

  push_pair({0, 0});
}

template <size_t D>
void huffmanAddWeights(const Block<RunLengthPair, D> &runlength, HuffmanWeights weights[2], size_t class_bits) {
  auto pairs_it = std::begin(runlength);

  pairs_it->addToWeights(weights[0], class_bits);

  do {
    pairs_it++;
    pairs_it->addToWeights(weights[1], class_bits);
  } while (!pairs_it->eob() && (pairs_it != (std::end(runlength) - 1)));
}

template <size_t D>
void encodeToStream(const Block<RunLengthPair, D> &runlength, const HuffmanEncoder encoder[2], OBitstream &stream, size_t class_bits) {
  auto pairs_it = std::begin(runlength);

  pairs_it->huffmanEncodeToStream(encoder[0], stream, class_bits);

  do {
    pairs_it++;
    pairs_it->huffmanEncodeToStream(encoder[1], stream, class_bits);
  } while (!pairs_it->eob() && (pairs_it != (std::end(runlength) - 1)));
}
#endif
