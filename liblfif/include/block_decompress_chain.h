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
#include "cabac.h"
#include "cabac_contexts.h"
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
void decodeFromStreamHuffman(IBitstream &bitstream, Block<RunLengthPair, BS, D> &runlength, const HuffmanDecoder huffman_decoders[2], size_t class_bits) {
  auto pairs_it = std::begin(runlength);

  pairs_it->huffmanDecodeFromStream(huffman_decoders[0], bitstream, class_bits);

  do {
    pairs_it++;
    pairs_it->huffmanDecodeFromStream(huffman_decoders[1], bitstream, class_bits);
  } while((pairs_it != (std::end(runlength) - 1)) && (!pairs_it->eob()));
}

/**
 * @brief Function which decodes one block of run-length pairs from stream which is encoded with CABAC.
 * @param bitstream  The input bitstream.
 * @param runlength  The output block of run-length pairs.
 * @param decoder    CABAC decoder.
 * @param models     Contexts for every bit of run-length value, amplitude size and amplitude value.
 * @param class_bits Number of bits for the second part of codeword.
 */
template <size_t BS, size_t D>
void decodeFromStreamCABAC(Block<RunLengthPair, BS, D> &runlength, CABACDecoder &decoder, CABAC::ContextModel models[(8+8+14) * 2], size_t class_bits) {
  auto pairs_it = std::begin(runlength);

  pairs_it->CABACDecodeFromStream(decoder, &models[0], class_bits);

  do {
    pairs_it++;
    pairs_it->CABACDecodeFromStream(decoder, &models[30], class_bits);
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
 * @brief Function which decodes one block of traversed coefficients from stream which is encoded with CABAC.
 * @param traversed_block The output block of traversed coefficients.
 * @param decoder    CABAC decoder.
 * @param contexts Contexts for block decoding.
 */
template <size_t BS, size_t D>
void decodeTraversedCABAC(Block<QDATAUNIT, BS, D> &traversed_block, CABACDecoder &decoder, CABACContexts<BS, D> &contexts) {

  traversed_block.fill(0);

  if (decoder.decodeBit(contexts.coded_block_flag_ctx)) {
    bool no_last_sig_coef_flag { true };

    for (size_t i = 0; i < constpow(BS, D) - 1; i++) {
      if (decoder.decodeBit(contexts.significant_coef_flag_ctx[i])) {
        traversed_block[i] = 1;

        if (decoder.decodeBit(contexts.last_significant_coef_flag_ctx[i])) {
          no_last_sig_coef_flag = false;
          break;
        }
      }
      else {
        traversed_block[i] = 0;
      }
    }

    if (no_last_sig_coef_flag) {
      traversed_block[constpow(BS, D) - 1] = 1;
    }

    size_t numT1   {0};
    size_t numLgt1 {0};

    for (size_t i = 1; i <= constpow(BS, D); i++) {
      size_t ii = constpow(BS, D) - i;

      QDATAUNIT coef = traversed_block[ii];

      if (coef != 0) {
        coef += decoder.decodeBit(contexts.coef_greater_one_ctx[std::min(numT1, contexts.NUM_GREATER_ONE_CTXS - 1)]);

        if (coef == 2) {
          coef += decoder.decodeUEG0(13, contexts.coef_abs_level_ctx[std::min(numLgt1, contexts.NUM_ABS_LEVEL_CTXS - 1)]);

          numT1 = contexts.NUM_GREATER_ONE_CTXS - 1;
          numLgt1++;
        }
        else if (numT1 < contexts.NUM_GREATER_ONE_CTXS - 2) {
          numT1++;
        }

        if (decoder.decodeBitBypass()) {
          coef = -coef;
        }

        traversed_block[ii] = coef;
      }
    }
  }
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
 * @brief Function which performs inverse DPCM.
 * @param diff_encoded_block The block which is DPCM encoded.
 * @param previous_DC Reference to value of previous DC coefficient.
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
