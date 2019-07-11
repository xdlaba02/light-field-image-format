/**
* @file block_compress_chain.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for image compression in a block scope.
*/


#ifndef BLOCK_COMPRESS_CHAIN
#define BLOCK_COMPRESS_CHAIN

#include "lfiftypes.h"
#include "quant_table.h"
#include "traversal_table.h"
#include "huffman.h"
#include "cabac.h"
#include "dct.h"
#include "block.h"
#include "runlength.h"

#include <cstdlib>
#include <cstdint>

/**
 * @brief Function which performs the forward DCT to the block. Output coefficients are returned in second parameter.
 * @param input_block The block to be transformed.
 * @param transformed_block The transformed block.
 */
template <size_t BS, size_t D>
void forwardDiscreteCosineTransform(const Block<INPUTUNIT, BS, D> &input_block, Block<DCTDATAUNIT, BS, D> &transformed_block) {
  transformed_block.fill(0);

  auto inputF = [&](size_t index) {
    return input_block[index];
  };

  auto outputF = [&](size_t index) -> auto & {
    return transformed_block[index];
  };

  fdct<BS, D>(inputF, outputF);
}

/**
 * @brief Function which performs the quantization step on a block.
 * @param transformed_block The block of DCT coefficients to be quantized.
 * @param quantized_block The quantized block.
 * @param quant_table The quantization matrix.
 */
template <size_t BS, size_t D>
void quantize(const Block<DCTDATAUNIT, BS, D> &transformed_block, Block<QDATAUNIT, BS, D> &quantized_block, const QuantTable<BS, D> &quant_table) {
  for (size_t i = 0; i < constpow(BS, D); i++) {
    quantized_block[i] = std::round(transformed_block[i] / quant_table[i]);
  }
}

/**
 * @brief Function which adds absolute values of coefficient to the reference block.
 * @param quantized_block The quantized block to be added.
 * @param reference The block to which values should be added.
 */
template <size_t BS, size_t D>
void addToReferenceBlock(const Block<QDATAUNIT, BS, D> &quantized_block, ReferenceBlock<BS, D> &reference) {
  for (size_t i = 0; i < constpow(BS, D); i++) {
    reference[i] += abs(quantized_block[i]);
  }
}

/**
 * @brief Function which performs DPCM encoding to the DC coefficient.
 * @param quantized_block The quantized block to be DPCM encoded.
 * @param previous_DC Value of a previous DC coefficient.
 */
template <size_t BS, size_t D>
void diffEncodeDC(Block<QDATAUNIT, BS, D> &quantized_block, QDATAUNIT &previous_DC) {
  QDATAUNIT current_DC = quantized_block[0];
  quantized_block[0] -= previous_DC;
  previous_DC = current_DC;
}

/**
 * @brief Function which rearranges the coefficients in memeory by traversal matrix.
 * @param diff_encoded_block The block to be traversed.
 * @param traversal_table The traversal matrix.
 */
template <size_t BS, size_t D>
void traverse(Block<QDATAUNIT, BS, D> &diff_encoded_block, const TraversalTable<BS, D> &traversal_table) {
  Block<QDATAUNIT, BS, D> diff_encoded_copy(diff_encoded_block);

  for (size_t i = 0; i < constpow(BS, D); i++) {
    diff_encoded_block[traversal_table[i]] = diff_encoded_copy[i];
  }
}

/**
 * @brief Function which performs run-length encoding
 * @param traversed_block The traversed block to be run-length encoded.
 * @param runlength The output block of run-length pairs, ended by EOB if not full.
 * @param max_zeroes Maximum number of zeroes in run-length.
 */
template <size_t BS, size_t D>
void runLengthEncode(const Block<QDATAUNIT, BS, D> &traversed_block, Block<RunLengthPair, BS, D> &runlength, size_t max_zeroes) {
  auto pairs_it = std::begin(runlength);

  auto push_pair = [&](RunLengthPair &&pair) {
    if (pairs_it != std::end(runlength)) {
      *pairs_it = pair;
      pairs_it++;
    }
  };

  push_pair({0, traversed_block[0]});

  size_t zeroes = 0;
  for (size_t i = 1; i < constpow(BS, D); i++) {
    if (traversed_block[i] == 0) {
      zeroes++;
    }
    else {
      while (zeroes > max_zeroes) {
        push_pair({max_zeroes, 0});
        zeroes -= max_zeroes + 1;
      }
      push_pair({zeroes, traversed_block[i]});
      zeroes = 0;
    }
  }

  push_pair({0, 0});
}

/**
 * @brief Function which weights symbols for huffman encoder.
 * @param runlength The input block of run-length pairs.
 * @param weights Two maps for huffman weighting. First is for DC coefficient, second for AC coefficients.
 * @param class_bits Number of bits for the second part of codeword.
 */
template <size_t BS, size_t D>
void huffmanAddWeights(const Block<RunLengthPair, BS, D> &runlength, HuffmanWeights weights[2], size_t class_bits) {
  auto pairs_it = std::begin(runlength);

  pairs_it->addToWeights(weights[0], class_bits);

  do {
    pairs_it++;
    pairs_it->addToWeights(weights[1], class_bits);
  } while (!pairs_it->eob() && (pairs_it != (std::end(runlength) - 1)));
}

/**
 * @brief Function encodes pairs to stream by initialized Huffman encoder.
 * @param runlength The input block of run-length pairs.
 * @param encoder Two run-length encoders. First is for DC coefficient, second is for AC coefficients.
 * @param stream The output bitstream.
 * @param class_bits Number of bits for the second part of codeword.
 */
template <size_t BS, size_t D>
void encodeToStream(const Block<RunLengthPair, BS, D> &runlength, const HuffmanEncoder encoder[2], OBitstream &stream, size_t class_bits) {
  auto pairs_it = std::begin(runlength);

  pairs_it->huffmanEncodeToStream(encoder[0], stream, class_bits);

  do {
    pairs_it++;
    pairs_it->huffmanEncodeToStream(encoder[1], stream, class_bits);
  } while (!pairs_it->eob() && (pairs_it != (std::end(runlength) - 1)));
}
#endif
