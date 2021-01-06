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
#include "predict.h"

#include <vector>

/**
 * @brief Function which decodes one block of run-length pairs from stream.
 * @param bitstream        The input bitstream.
 * @param runlength        The output block of run-length pairs.
 * @param huffman_decoders Two huffman decoders. First is for DC coefficient, second for AC coefficients.
 * @param class_bits       Number of bits for the second part of codeword.
 */
template <size_t D>
void decodeHuffman_RUNLENGTH(IBitstream &bitstream, DynamicBlock<RunLengthPair, D> &runlength, const HuffmanDecoder huffman_decoders[2], size_t class_bits) {
  auto pairs_it = &runlength[0];

  pairs_it->huffmanDecodeFromStream(huffman_decoders[0], bitstream, class_bits);

  do {
    pairs_it++;
    pairs_it->huffmanDecodeFromStream(huffman_decoders[1], bitstream, class_bits);
  } while ((pairs_it != (&runlength[runlength.size()] - 1)) && (!pairs_it->eob()));
}

template <size_t D>
void decodePredictionType(uint64_t &prediction_type, CABACDecoder &decoder, CABACContextsPredictionMode<D> &contexts) {
  prediction_type = 0;
  while (decoder.decodeBit(contexts.prediction_ctx[prediction_type])) {
    prediction_type++;
  }
}

template <size_t D>
void decodeCABAC_DIAGONAL(DynamicBlock<QDATAUNIT, D> &diff_encoded_block, CABACDecoder &decoder, CABACContextsDIAGONAL<D> &contexts, size_t &threshold, const std::vector<std::vector<size_t>> &scan_table) {
  diff_encoded_block.fill(0);

  std::vector<bool> nonzero_diags(num_diagonals<D>(diff_encoded_block.size()));

  for (size_t diag = 0; diag < num_diagonals<D>(diff_encoded_block.size()); diag++) {
    nonzero_diags[diag] = decoder.decodeBit(contexts.coded_diag_flag_ctx[diag]);

    if (nonzero_diags[diag]) {
      if (decoder.decodeBit(contexts.last_coded_diag_flag_ctx[diag])) {
        break;
      }
    }
  }


  for (size_t d { 1 }; d <= num_diagonals<D>(diff_encoded_block.size()); d++) {
    size_t diag = num_diagonals<D>(diff_encoded_block.size()) - d;
    int64_t zero_coef_distr { 0 };

    if (nonzero_diags[diag]) {
      for (auto &i: scan_table[diag]) {
        QDATAUNIT coef {};

        size_t nonzero_neighbours_cnt { 0 };

        for (size_t dim = 0; dim < D; dim++) {
          size_t neighbour = i + diff_encoded_block.stride(dim);
          if ((i % diff_encoded_block.stride(dim + 1)) < (neighbour % diff_encoded_block.stride(dim + 1))) {
            nonzero_neighbours_cnt += (diff_encoded_block[neighbour] != 0);
          }
        }

        if (diag < threshold) {
          coef = decoder.decodeBit(contexts.significant_coef_flag_high_ctx[nonzero_neighbours_cnt]);
        }
        else {
          coef = decoder.decodeBit(contexts.significant_coef_flag_low_ctx[nonzero_neighbours_cnt]);
        }

        if (!coef) {
          zero_coef_distr++;
        }
        else {
          zero_coef_distr--;

          coef += decoder.decodeBit(contexts.coef_greater_one_ctx[diag]);

          if (coef > 1) {
            coef += decoder.decodeBit(contexts.coef_greater_two_ctx[diag]);

            if (coef > 2) {
              coef += decoder.decodeU(contexts.coef_abs_level_ctx[diag]);
            }
          }

          if (decoder.decodeBitBypass()) {
            coef = -coef;
          }
        }

        diff_encoded_block[i] = coef;
      }
    }

    if ((zero_coef_distr > 0) && (threshold > diag)) {
      threshold--;
    }
    else if ((zero_coef_distr < 0) && (threshold < diag)) {
      threshold++;
    }
  }
}


/**
 * @brief Function which expands the run-length encoded block into the traversed block of coefficients.
 * @param runlength       The input block of run-length pairs.
 * @param traversed_block The output block of coefficients which are traversed in memory.
 */
template <size_t D>
void runLengthDecode(const DynamicBlock<RunLengthPair, D> &runlength, DynamicBlock<QDATAUNIT, D> &traversed_block) {
  assert(runlength.size() == traversed_block.size());

  traversed_block.fill(0);

  auto block_it = &traversed_block[0];
  auto pairs_it = &runlength[0];

  do {
    block_it += pairs_it->zeroes;
    *block_it = pairs_it->amplitude;

    block_it++;
    pairs_it++;
  } while ((pairs_it != &runlength[runlength.size()]) && (!pairs_it->eob()) && (block_it != &traversed_block[traversed_block.size()]));
}

/**
 * @brief Function which decodes one block of traversed coefficients from stream which is encoded with CABAC.
 * @param traversed_block The output block of traversed coefficients.
 * @param decoder    CABAC decoder.
 * @param contexts Contexts for block decoding.
 */
template <size_t D>
void decodeCABAC_H264(DynamicBlock<QDATAUNIT, D> &traversed_block, CABACDecoder &decoder, CABACContextsH264<D> &contexts) {
  traversed_block.fill(0);

  if (decoder.decodeBit(contexts.coded_block_flag_ctx)) {
    bool no_last_sig_coef_flag { true };

    for (size_t i = 0; i < traversed_block.stride(D) - 1; i++) {
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
      traversed_block[traversed_block.stride(D) - 1] = 1;
    }

    size_t numT1   {0};
    size_t numLgt1 {0};

    for (size_t i = 1; i <= traversed_block.stride(D); i++) {
      size_t ii = traversed_block.stride(D) - i;

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
template <size_t D>
void detraverse(DynamicBlock<QDATAUNIT, D> &traversed_block, const TraversalTable<D> &traversal_table) {
  assert(traversed_block.size() == traversal_table.size());

  DynamicBlock<QDATAUNIT, D> traversed_block_copy(traversed_block);

  for (size_t i = 0; i < traversed_block.stride(D); i++) {
    traversed_block[i] = traversed_block_copy[traversal_table[i]];
  }
}

/**
 * @brief Function which performs inverse DPCM.
 * @param diff_encoded_block The block which is DPCM encoded.
 * @param previous_DC Reference to value of previous DC coefficient.
 */
template <size_t D>
void diffDecodeDC(DynamicBlock<QDATAUNIT, D> &diff_encoded_block, QDATAUNIT &previous_DC) {
  diff_encoded_block[0] += previous_DC;
  previous_DC = diff_encoded_block[0];
}

/**
 * @brief Function which performs inverse quantization step.
 * @param quantized_block The input block of quantized coefficients.
 * @param dct_block       The output block of the DCT coefficients.
 * @param quant_table     The quantization matrix.
 */
template <size_t D>
void dequantize(const DynamicBlock<QDATAUNIT, D> &quantized_block, DynamicBlock<DCTDATAUNIT, D> &dct_block, const QuantTable<D> &quant_table) {
  assert(quantized_block.size() == dct_block.size());
  assert(dct_block.size() == quant_table.size());

  for (size_t i = 0; i < quantized_block.stride(D); i++) {
    dct_block[i] = static_cast<DCTDATAUNIT>(quantized_block[i]) * quant_table[i];
  }
}

/**
 * @brief Function which performs inverse discrete cosine tranformation.
 * @param dct_block    The input block of the DCT coefficients.
 * @param output_block The output block of image samples.
 */
template <size_t D>
void inverseDiscreteCosineTransform(const DynamicBlock<DCTDATAUNIT, D> &dct_block, DynamicBlock<INPUTUNIT, D> &output_block) {
  assert(dct_block.size() == output_block.size());

  output_block.fill(0);

  auto inputF = [&](size_t index) {
    return dct_block[index];
  };

  auto outputF = [&](size_t index) -> auto & {
    return output_block[index];
  };

  idct<D>(dct_block.size().data(), inputF, outputF);
}

template <size_t D>
void disusePrediction(DynamicBlock<INPUTUNIT, D> &input_block, const DynamicBlock<INPUTUNIT, D> &prediction_block) {
  assert(input_block.size() == prediction_block.size());

  iterate_dimensions<D>(input_block.size(), [&](const auto &pos) {
    input_block[pos] += prediction_block[pos];
  });
}

#endif
