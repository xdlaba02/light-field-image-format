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
#include "cabac_contexts.h"
#include "dct.h"
#include "block.h"
#include "runlength.h"

#include <cstdlib>
#include <cstdint>

#include <algorithm>

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
 * @brief Function encodes traversed block to stream by CABAC encoder.
 * @param traversed_block The input block of traversed coefficients.
 * @param encoder CABAC Encoder.
 * @param contexts Contexts for block encoding.
 */
template <size_t BS, size_t D>
void encodeTraversedCABAC(const Block<QDATAUNIT, BS, D> &traversed_block, CABACEncoder &encoder, CABACContexts<BS, D> &contexts) {
  size_t coef_cnt {};

  for (size_t i = 0; i < constpow(BS, D); i++) {
    if (traversed_block[i]) {
      coef_cnt++;
    }
  }

  if (coef_cnt > 0) {
    encoder.encodeBit(contexts.coded_block_flag_ctx, 1);

    for (size_t i = 0; i < constpow(BS, D) - 1; i++) {
      if (traversed_block[i] == 0) {
        encoder.encodeBit(contexts.significant_coef_flag_ctx[i], 0);
      }
      else {
        encoder.encodeBit(contexts.significant_coef_flag_ctx[i], 1);
        coef_cnt--;

        if (coef_cnt == 0) {
          encoder.encodeBit(contexts.last_significant_coef_flag_ctx[i], 1);
          break;
        }
        else {
          encoder.encodeBit(contexts.last_significant_coef_flag_ctx[i], 0);
        }
      }
    }

    size_t numT1   {0};
    size_t numLgt1 {0};

    for (size_t i = 1; i <= constpow(BS, D); i++) {
      size_t ii = constpow(BS, D) - i;

      QDATAUNIT coef = traversed_block[ii];

      if (coef != 0) {
        bool sign {};

        if (coef > 0) {
          sign = 0;
        }
        else {
          sign = 1;
          coef = -coef;
        }

        bool greater_one {};

        if (coef > 1) {
          greater_one = true;
        }
        else {
          greater_one = false;
        }

        encoder.encodeBit(contexts.coef_greater_one_ctx[numT1], greater_one);

        if (greater_one) {
          encoder.encodeUEG0(13, contexts.coef_abs_level_ctx[std::min(numLgt1, contexts.NUM_ABS_LEVEL_CTXS - 1)], coef - 2);

          numT1 = contexts.NUM_GREATER_ONE_CTXS - 1;
          numLgt1++;
        }
        else if (numT1 < contexts.NUM_GREATER_ONE_CTXS - 2) {
          numT1++;
        }

        encoder.encodeBitBypass(sign);
      }
    }
  }
  else {
    encoder.encodeBit(contexts.coded_block_flag_ctx, 0);
  }
}

/**
 * @brief Function encodes traversed block to stream by CABAC encoder.
 * @param traversed_block The input block of traversed coefficients.
 * @param encoder CABAC Encoder.
 * @param contexts Contexts for block encoding.
 */
template <size_t BS, size_t D>
void encodeTraversedCABACJPEG(const Block<QDATAUNIT, BS, D> &traversed_block, CABACEncoder &encoder, CABACContextsJPEG<BS, D> &contexts, QDATAUNIT &previous_DC_diff, size_t amp_bits) {
  QDATAUNIT coef     {};
  size_t    S        {};
  size_t    coef_cnt { 0 };

  if (previous_DC_diff < -2) {
    S = 0;
  }
  else if (previous_DC_diff > 2) {
    S = 4;
  }
  else if (previous_DC_diff < 0) {
    S = 8;
  }
  else if (previous_DC_diff > 0) {
    S = 12;
  }
  else {
    S = 16;
  }

  previous_DC_diff = traversed_block[0];

  for (size_t i = 1; i < constpow(BS, D); i++) {
    if (traversed_block[i]) {
      coef_cnt++;
    }
  }

  coef = traversed_block[0];

  if (!coef) {
    encoder.encodeBit(contexts.DC_ctxs[S], 0);
  }
  else {
    encoder.encodeBit(contexts.DC_ctxs[S], 1);

    bool sign {};

    if (coef < 0) {
      encoder.encodeBit(contexts.DC_ctxs[S + 1], 1);
      coef = -coef;
      sign = 1;
      S += 2;
    }
    else {
      encoder.encodeBit(contexts.DC_ctxs[S + 1], 0);
      sign = 0;
      S += 3;
    }

    size_t M { 1 };

    if (coef > M) {
      encoder.encodeBit(contexts.DC_ctxs[S], 1);

      M = 2;
      S = 20;
      while (coef > M) {
        encoder.encodeBit(contexts.DC_ctxs[S], 1);
        M <<= 1;
        S++;
      }

      encoder.encodeBit(contexts.DC_ctxs[S], 0);
    }
    else {
      encoder.encodeBit(contexts.DC_ctxs[S], 0);
    }

    S += amp_bits - 1;
    M >>= 2;

    while (M) {
      if ((coef - 1) & M) {
        encoder.encodeBit(contexts.DC_ctxs[S], 1);
      }
      else {
        encoder.encodeBit(contexts.DC_ctxs[S], 0);
      }

      M >>= 1;
    }
  }

  for (size_t i = 1; i < constpow(BS, D); i++) {
    S = 3 * (i - 1);
    if (coef_cnt == 0) {
      encoder.encodeBit(contexts.AC_ctxs[S], 1);
      break;
    }
    else {
      encoder.encodeBit(contexts.AC_ctxs[S], 0);
    }

    coef = traversed_block[i];

    if (!coef) {
      encoder.encodeBit(contexts.AC_ctxs[S + 1], 0);
    }
    else {
      encoder.encodeBit(contexts.AC_ctxs[S + 1], 1);

      if (coef < 0) {
        encoder.encodeBitBypass(1);
        coef = -coef;
      }
      else {
        encoder.encodeBitBypass(0);
      }

      size_t M { 1 };

      if (coef > M) {
        encoder.encodeBit(contexts.AC_ctxs[S + 2], 1);
        M <<= 1;

        if (coef > M) {
          encoder.encodeBit(contexts.AC_ctxs[S + 2], 1);
          M <<= 1;

          S = 3 * (constpow(BS, D) - 1);
          if (i > 32) {
            S += 2 * (amp_bits - 1);
          }

          while (coef > M) {
            encoder.encodeBit(contexts.AC_ctxs[S], 1);
            M <<= 1;
            S++;
          }

          encoder.encodeBit(contexts.AC_ctxs[S], 0);
        }
        else {
          encoder.encodeBit(contexts.AC_ctxs[S + 2], 0);
        }
      }
      else {
        encoder.encodeBit(contexts.AC_ctxs[S + 2], 0);
      }

      M >>= 2;
      S += amp_bits - 1;

      while (M) {
        if ((coef - 1) & M) {
          encoder.encodeBit(contexts.AC_ctxs[S], 1);
        }
        else {
          encoder.encodeBit(contexts.AC_ctxs[S], 0);
        }

        M >>= 1;
      }

      coef_cnt--;
    }
  }
}

/**
 * @brief Function encodes traversed block to stream by CABAC encoder.
 * @param traversed_block The input block of traversed coefficients.
 * @param encoder CABAC Encoder.
 * @param contexts Contexts for block encoding.
 */
template <size_t BS, size_t D>
void encodeCABACDIAGONAL(const Block<QDATAUNIT, BS, D> &diff_encoded_block, CABACEncoder &encoder, CABACContextsDIAGONAL<BS, D> &contexts, size_t &threshold) {
  std::array<bool, D * (BS - 1) + 1> nonzero_diags {};
  size_t diags_cnt { 0 };

  for (size_t i = 0; i < constpow(BS, D); i++) {
    size_t diagonal { 0 };
    for (size_t j = i; j; j /= BS) {
      diagonal += j % BS;
    }

    if (diff_encoded_block[i]) {
      nonzero_diags[diagonal] = true;
    }
  }

  for (auto &nonzero: nonzero_diags) {
    if (nonzero) {
      diags_cnt++;
    }
  }

  for (size_t diag = 0; diag < D * (BS - 1) + 1; diag++) {
    encoder.encodeBit(contexts.coded_diag_flag_ctx[diag], nonzero_diags[diag]);

    if (nonzero_diags[diag]) {
      diags_cnt--;

      if (diags_cnt) {
        encoder.encodeBit(contexts.last_coded_diag_flag_ctx[diag], 0);
      }
      else {
        encoder.encodeBit(contexts.last_coded_diag_flag_ctx[diag], 1);
        break;
      }
    }
  }

  int64_t zero_coef_distr { 0 };

  for (size_t d = 0; d < D * (BS - 1) + 1; d++) {

    size_t diag = (D * (BS - 1)) - d;

    if (nonzero_diags[diag]) {
      for (size_t i = 0; i < constpow(BS, D); i++) {
        size_t diagonal { 0 };

        for (size_t j = i; j; j /= BS) {
          diagonal += j % BS;
        }

        if (diagonal == diag) {
          QDATAUNIT coef = diff_encoded_block[i];

          size_t nonzero_neighbours_cnt { 0 };

          for (size_t dim = 0; dim < D; dim++) {
            size_t neighbour = i + constpow(BS, dim);
            if (neighbour < constpow(BS, D)) {
              nonzero_neighbours_cnt += (diff_encoded_block[neighbour] != 0);
            }
          }

          if (diag < threshold) {
            encoder.encodeBit(contexts.significant_coef_flag_high_ctx[nonzero_neighbours_cnt], coef);
          }
          else {
            encoder.encodeBit(contexts.significant_coef_flag_low_ctx[nonzero_neighbours_cnt], coef);
          }

          if (!coef) {
            zero_coef_distr++;
          }
          else {
            zero_coef_distr--;

            bool sign {};

            if (coef > 0) {
              sign = 0;
            }
            else {
              sign = 1;
              coef = -coef;
            }

            encoder.encodeBit(contexts.coef_greater_one_ctx[diag], coef > 1);

            if (coef > 1) {
              encoder.encodeBit(contexts.coef_greater_two_ctx[diag], coef > 2);

              if (coef > 2) {
                encoder.encodeU(contexts.coef_abs_level_ctx[diag], coef - 3);
              }
            }

            encoder.encodeBitBypass(sign);
          }
        }
      }
    }

    if ((zero_coef_distr > 0) && (threshold > diag)) {
      threshold--;
    }
    else if ((zero_coef_distr < 0) && (threshold < diag)) {
      threshold++;
    }

    zero_coef_distr = 0;
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
void encodeToStreamHuffman(const Block<RunLengthPair, BS, D> &runlength, const HuffmanEncoder encoder[2], OBitstream &stream, size_t class_bits) {
  auto pairs_it = std::begin(runlength);

  pairs_it->huffmanEncodeToStream(encoder[0], stream, class_bits);

  do {
    pairs_it++;
    pairs_it->huffmanEncodeToStream(encoder[1], stream, class_bits);
  } while (!pairs_it->eob() && (pairs_it != (std::end(runlength) - 1)));
}

/**
 * @brief Function encodes pairs to stream by CABAC encoder.
 * @param runlength The input block of run-length pairs.
 * @param encoder CABAC Encoder.
 * @param models Contexts for every bit of run-length value, amplitude size and amplitude value.
 * @param class_bits Number of bits for the second part of codeword.
 */
template <size_t BS, size_t D>
void encodeToStreamCABAC(const Block<RunLengthPair, BS, D> &runlength, CABACEncoder &encoder, CABAC::ContextModel models[(8+8+14) * 2], size_t class_bits) {
  auto pairs_it = std::begin(runlength);

  pairs_it->CABACEncodeToStream(encoder, &models[0], class_bits);

  do {
    pairs_it++;
    pairs_it->CABACEncodeToStream(encoder, &models[(8+8+14)], class_bits);
  } while (!pairs_it->eob() && (pairs_it != (std::end(runlength) - 1)));
}

#endif
