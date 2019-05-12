/**
* @file block_decompress_chain.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for image decompression in a block scope.
*/

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

/**
 * @brief Function which decodes one block of run-length pairs from stream.
 * @param bitstream        The input bitstream.
 * @param runlength        The output block of run-length pairs.
 * @param huffman_decoders Two huffman decoders. First is for DC coefficient, second for AC coefficients.
 * @param class_bits       Number of bits for the second part of codeword.
 */
template <size_t BS, size_t D>
void decodeFromStream(IBitstream &bitstream, Block<RunLengthPair, BS, D> &runlength, const HuffmanDecoder huffman_decoders[2], size_t class_bits) {
  auto pairs_it = std::begin(runlength);

  pairs_it->huffmanDecodeFromStream(huffman_decoders[0], bitstream, class_bits);

  do {
    pairs_it++;
    pairs_it->huffmanDecodeFromStream(huffman_decoders[1], bitstream, class_bits);
  } while((pairs_it != (std::end(runlength) - 1)) && (!pairs_it->eob()));
}

/**
 * @brief Function which expands the run-length encoded block into the traversed block of coefficients.
 * @param runlength       The input block of run-length pairs.
 * @param traversed_block The output block of coefficients which are traversed in memory.
 */
template <size_t BS, size_t D>
void runLengthDecode(const Block<RunLengthPair, BS, D> &runlength, Block<QDATAUNIT, BS, D> &traversed_block) {
  traversed_block.fill(0);

  auto block_it = std::begin(traversed_block);
  auto pairs_it = std::begin(runlength);

  do {
    block_it += pairs_it->zeroes;
    *block_it = pairs_it->amplitude;

    block_it++;
    pairs_it++;
  } while ((pairs_it != std::end(runlength)) && (!pairs_it->eob()) && (block_it != std::end(traversed_block)));
}

/**
 * @brief Function which detraverses the traversed block of coefficients.
 * @param traversed_block The input block of coefficients which are traversed in memory.
 * @param traversal_table The traversal matrix.
 */
template <size_t BS, size_t D>
void detraverse(Block<QDATAUNIT, BS, D> &traversed_block, const TraversalTable<BS, D> &traversal_table) {
  Block<QDATAUNIT, BS, D> traversed_block_copy(traversed_block);

  for (size_t i = 0; i < constpow(BS, D); i++) {
    traversed_block[i] = traversed_block_copy[traversal_table[i]];
  }
}

/**
 * @brief Function which detraverses the traversed block of coefficients.
 * @param traversed_block The input block of coefficients which are traversed in memory.
 * @param traversal_table The traversal matrix.
 */
template <size_t BS, size_t D>
void diffDecodeDC(Block<QDATAUNIT, BS, D> &diff_encoded_block, QDATAUNIT &previous_DC) {
  diff_encoded_block[0] += previous_DC;
  previous_DC = diff_encoded_block[0];
}

/**
 * @brief Function which performs inverse quantization step.
 * @param quantized_block The input block of quantized coefficients.
 * @param dct_block       The output block of the DCT coefficients.
 * @param quant_table     The quantization matrix.
 */
template <size_t BS, size_t D>
void dequantize(const Block<QDATAUNIT, BS, D> &quantized_block, Block<DCTDATAUNIT, BS, D> &dct_block, const QuantTable<BS, D> &quant_table) {
  for (size_t i = 0; i < constpow(BS, D); i++) {
    dct_block[i] = static_cast<DCTDATAUNIT>(quantized_block[i]) * quant_table[i];
  }
}

/**
 * @brief Function which performs inverse discrete cosine tranformation.
 * @param dct_block    The input block of the DCT coefficients.
 * @param output_block The output block of image samples.
 */
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
