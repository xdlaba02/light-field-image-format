/**
* @file block_operations.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 20. 1. 2021
* @copyright 2021 Drahomír Dlabaja
* @brief Functions for image compression and decompression in a block scope.
*/

#pragma once

#include "quant_table.h"
#include "cabac.h"
#include "contexts.h"
#include "dct.h"
#include "block.h"
#include "predict.h"

#include <cstdlib>
#include <cstdint>

#include <algorithm>

/**
 * @brief Function which performs the forward DCT to the block. Output coefficients are returned in second parameter.
 * @param input_block The block to be transformed.
 * @param transformed_block The transformed block.
 */
template <size_t D>
void forwardDiscreteCosineTransform(DynamicBlock<float, D> &block) {
  auto proxy = [&](size_t index) -> auto & {
    return block[index];
  };


  fdct<D>(block.size(), proxy);
}

/**
 * @brief Function which performs inverse discrete cosine tranformation.
 * @param dct_block    The input block of the DCT coefficients.
 * @param output_block The output block of image samples.
 */
template <size_t D>
void inverseDiscreteCosineTransform(DynamicBlock<float, D> &block) {
  auto proxy = [&](size_t index) -> auto & {
    return block[index];
  };

  idct<D>(block.size(), proxy);
}

/**
 * @brief Function which performs the quantization step on a block.
 * @param transformed_block The block of DCT coefficients to be quantized.
 * @param quantized_block The quantized block.
 * @param quant_table The quantization matrix.
 */
template <size_t D>
void quantize(const DynamicBlock<float, D> &input, DynamicBlock<int64_t, D> &output, uint8_t shift_val) {
  assert(input.size() == output.size());
  for (size_t i = 0; i < input.stride(D); i++) {
    output[i] = std::round(ldexp(input[i], -shift_val));
  }
}

/**
 * @brief Function which performs inverse quantization step.
 * @param quantized_block The input block of quantized coefficients.
 * @param dct_block       The output block of the DCT coefficients.
 * @param quant_table     The quantization matrix.
 */
template <size_t D>
void dequantize(const DynamicBlock<int64_t, D> &input, DynamicBlock<float, D> &output, uint8_t shift_val) {
  assert(input.size() == output.size());

  for (size_t i = 0; i < input.stride(D); i++) {
    output[i] = input[i] << shift_val;
  }
}

template <size_t D>
void encodePredictionType(uint64_t prediction_type, CABACEncoder &encoder, PredictionModeContexts<D> &contexts) {
  size_t i {};
  for (i = 0; i < prediction_type; i++) {
    encoder.encodeBit(contexts.prediction_ctx[i], 1);
  }
  encoder.encodeBit(contexts.prediction_ctx[i], 0);
}

template <size_t D>
void decodePredictionType(uint64_t &prediction_type, CABACDecoder &decoder, PredictionModeContexts<D> &contexts) {
  prediction_type = 0;
  while (decoder.decodeBit(contexts.prediction_ctx[prediction_type])) {
    prediction_type++;
  }
}


/**
 * @brief Function encodes block block to stream by CABAC encoder.
 * @param traversed_block The input block of traversed coefficients.
 * @param encoder CABAC Encoder.
 * @param contexts Contexts for block encoding.
 */
template <size_t D>
void encodeBlock(const DynamicBlock<int64_t, D> &input, CABACEncoder &encoder, ThresholdedDiagonalContexts<D> &contexts, const std::vector<std::vector<size_t>> &scan_table) {
  std::vector<bool> nonzero_diags(num_diagonals<D>(input.size()));
  size_t diags_cnt { 0 };

  for (size_t diag = 0; diag < num_diagonals<D>(input.size()); diag++) {
    for (auto &i: scan_table[diag]) {
      if (input[i]) {
        nonzero_diags[diag] = true;
        diags_cnt++;
        break;
      }
    }
  }

  for (size_t diag = 0; diag < num_diagonals<D>(input.size()); diag++) {
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

  for (size_t d = 1; d <= num_diagonals<D>(input.size()); d++) {
    size_t diag = num_diagonals<D>(input.size()) - d;
    int64_t zero_coef_distr { 0 };

    if (nonzero_diags[diag]) {
      for (auto &i: scan_table[diag]) {
        int64_t coef = input[i];

        size_t nonzero_neighbours_cnt { 0 };

        for (size_t dim = 0; dim < D; dim++) {
          size_t neighbour = i + input.stride(dim);
          if ((i % input.stride(dim + 1)) < (neighbour % input.stride(dim + 1))) {
            nonzero_neighbours_cnt += (input[neighbour] != 0);
          }
        }

        if (diag < contexts.threshold) {
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

    if ((zero_coef_distr > 0) && (contexts.threshold > diag)) {
      contexts.threshold--;
    }
    else if ((zero_coef_distr < 0) && (contexts.threshold < diag)) {
      contexts.threshold++;
    }
  }
}

template <size_t D>
void decodeBlock(DynamicBlock<int64_t, D> &output, CABACDecoder &decoder, ThresholdedDiagonalContexts<D> &contexts, const std::vector<std::vector<size_t>> &scan_table) {
  output.fill(0);

  std::vector<bool> nonzero_diags(num_diagonals<D>(output.size()));

  for (size_t diag = 0; diag < num_diagonals<D>(output.size()); diag++) {
    nonzero_diags[diag] = decoder.decodeBit(contexts.coded_diag_flag_ctx[diag]);

    if (nonzero_diags[diag]) {
      if (decoder.decodeBit(contexts.last_coded_diag_flag_ctx[diag])) {
        break;
      }
    }
  }


  for (size_t d { 1 }; d <= num_diagonals<D>(output.size()); d++) {
    size_t diag = num_diagonals<D>(output.size()) - d;
    int64_t zero_coef_distr { 0 };

    if (nonzero_diags[diag]) {
      for (auto &i: scan_table[diag]) {
        int64_t coef {};

        size_t nonzero_neighbours_cnt { 0 };

        for (size_t dim = 0; dim < D; dim++) {
          size_t neighbour = i + output.stride(dim);
          if ((i % output.stride(dim + 1)) < (neighbour % output.stride(dim + 1))) {
            nonzero_neighbours_cnt += (output[neighbour] != 0);
          }
        }

        if (diag < contexts.threshold) {
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

        output[i] = coef;
      }
    }

    if ((zero_coef_distr > 0) && (contexts.threshold > diag)) {
      contexts.threshold--;
    }
    else if ((zero_coef_distr < 0) && (contexts.threshold < diag)) {
      contexts.threshold++;
    }
  }
}

template <size_t D, typename F>
uint64_t find_best_prediction_type(const DynamicBlock<float, D> &input_block, F &&inputF) {
  DynamicBlock<float, D> prediction_block(input_block.size());

  auto SAE = [](const auto &b) -> auto {
    float sae = 0;
    for (size_t i = 0; i < b.stride(D); i++) {
      sae += std::abs(b[i]);
    }
    return sae;
  };

  uint64_t best_prediction_type {};

  float lowest_sae {};
  float sae {};

  lowest_sae = SAE(input_block);

  int64_t predicted_dc = predict_DC<D>(prediction_block.size(), inputF);
  for (size_t i = 0; i < input_block.stride(D); i++) {
    prediction_block[i] = input_block[i] - predicted_dc;
  }

  sae = SAE(prediction_block);
  if (sae < lowest_sae) {
    lowest_sae = sae;
    best_prediction_type = 1;
  }

  predict_planar<D>(prediction_block, inputF);
  for (size_t i = 0; i < input_block.stride(D); i++) {
    prediction_block[i] = input_block[i] - prediction_block[i];
  }

  sae = SAE(prediction_block);
  if (sae < lowest_sae) {
    lowest_sae = sae;
    best_prediction_type = 2;
  }

  iterate_cube<5, D>([&](const std::array<size_t, D> &pos) {
    int8_t direction[D] {};

    for (size_t i { 0 }; i < D; i++) {
      direction[i] = pos[i] - 2;
    }

    auto have_positive = [&]() {
      for (size_t d { 0 }; d < D; d++) {
        if (direction[d] > 0) {
          return true;
        }
      }
      return false;
    };

    auto have_eight = [&]() {
      for (size_t d { 0 }; d < D; d++) {
        if (std::abs(direction[d]) == 2) {
          return true;
        }
      }
      return false;
    };

    if (!have_positive() || !have_eight()) {
      return;
    }

    predict_direction<D>(prediction_block, direction, inputF);
    for (size_t i = 0; i < input_block.stride(D); i++) {
      prediction_block[i] = input_block[i] - prediction_block[i];
    }

    sae = SAE(prediction_block);
    if (sae < lowest_sae) {
      lowest_sae = sae;
      best_prediction_type = make_cube_index<5, D>(pos) + 3;
    }
  });

  return best_prediction_type;
}

template <size_t D, typename F>
void predict(DynamicBlock<float, D> &prediction_block, uint64_t prediction_type, F &&inputF) {
  if (prediction_type == 1) {
    int64_t value = predict_DC<D>(prediction_block.size(), inputF);
    for (size_t i = 0; i < prediction_block.stride(D); i++) {
      prediction_block[i] = value;
    }
  }
  else if (prediction_type == 2) {
    predict_planar<D>(prediction_block, inputF);
  }
  else if (prediction_type >= 3) {
    size_t dir = prediction_type - 3;
    int8_t direction[D] {};

    for (size_t d { 0 }; d < D; d++) {
      direction[d] = dir % constpow(5, d + 1) / constpow(5, d);
      direction[d] -= 2;
    }

    predict_direction<D>(prediction_block, direction, inputF);
  }
}

template <size_t D>
void applyPrediction(DynamicBlock<float, D> &input_block, const DynamicBlock<float, D> &prediction_block) {
  assert(input_block.size() == prediction_block.size());

  iterate_dimensions<D>(input_block.size(), [&](const auto &pos) {
    input_block[pos] -= prediction_block[pos];
  });
}

template <size_t D>
void disusePrediction(DynamicBlock<float, D> &input_block, const DynamicBlock<float, D> &prediction_block) {
  assert(input_block.size() == prediction_block.size());

  iterate_dimensions<D>(input_block.size(), [&](const auto &pos) {
    input_block[pos] += prediction_block[pos];
  });
}
