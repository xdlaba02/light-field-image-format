#pragma once

#include "components/block.h"
#include "components/cabac.h"
#include "components/colorspace.h"
#include "components/meta.h"
#include "components/dct.h"
#include "lfif.h"

#include <cstdint>
#include <cstddef>

#include <array>

template<size_t D>
class BlockEncoderDCT {
  size_t diagonals;

  size_t threshold;
  std::vector<CABAC::ContextModel> significant_coef_flag_high_ctx;
  std::vector<CABAC::ContextModel> significant_coef_flag_low_ctx;

  std::vector<CABAC::ContextModel> coded_diag_flag_ctx;
  std::vector<CABAC::ContextModel> last_coded_diag_flag_ctx;
  std::vector<CABAC::ContextModel> coef_greater_one_ctx;
  std::vector<CABAC::ContextModel> coef_greater_two_ctx;
  std::vector<CABAC::ContextModel> coef_abs_level_ctx;

  std::vector<std::vector<size_t>> scan_table;

  std::array<size_t, D> block_size;
  uint8_t discarded_bits;

public:
  BlockEncoderDCT(const std::array<size_t, D> &block_size, uint8_t discarded_bits) {
    this->block_size = block_size;
    this->discarded_bits = discarded_bits;

    diagonals = num_diagonals<D>(block_size);

    scan_table.resize(diagonals);
    iterate_dimensions<D>(block_size, [&](const auto &pos) {
      size_t diagonal {};
      for (size_t i = 0; i < D; i++) {
        diagonal += pos[i];
      }

      scan_table[diagonal].push_back(make_index(block_size, pos));
    });


    threshold = diagonals / 2;

    coded_diag_flag_ctx.resize(diagonals);
    last_coded_diag_flag_ctx.resize(diagonals);
    significant_coef_flag_high_ctx.resize(D + 1);
    significant_coef_flag_low_ctx.resize(D + 1);
    coef_greater_one_ctx.resize(diagonals);
    coef_greater_two_ctx.resize(diagonals);
    coef_abs_level_ctx.resize(diagonals);
  }

  void encodeBlock(DynamicBlock<float, D> &block, CABACEncoder &encoder) {
    auto proxy = [&](size_t index) -> auto & {
      return block[index];
    };

    fdct<D>(block.size(), proxy);

    std::vector<bool> nonzero_diags(diagonals);
    size_t diags_cnt = 0;

    for (size_t diag = 0; diag < diagonals; diag++) {
      for (auto &i: scan_table[diag]) {
        if (block[i]) {
          nonzero_diags[diag] = true;
          diags_cnt++;
          break;
        }
      }
    }

    for (size_t diag = 0; diag < diagonals; diag++) {
      encoder.encodeBit(coded_diag_flag_ctx[diag], nonzero_diags[diag]);

      if (nonzero_diags[diag]) {
        diags_cnt--;

        if (diags_cnt) {
          encoder.encodeBit(last_coded_diag_flag_ctx[diag], 0);
        }
        else {
          encoder.encodeBit(last_coded_diag_flag_ctx[diag], 1);
          break;
        }
      }
    }

    for (size_t d = 1; d <= diagonals; d++) {
      size_t diag = diagonals - d;
      int64_t zero_coef_distr = 0;

      if (nonzero_diags[diag]) {
        for (auto &i: scan_table[diag]) {
          int64_t coef = std::round(ldexp(block[i], -discarded_bits)); // QUANTIZATION

          size_t nonzero_neighbours_cnt = 0;

          for (size_t dim = 0; dim < D; dim++) {
            size_t neighbour = i + block.stride(dim);
            if ((i % block.stride(dim + 1)) < (neighbour % block.stride(dim + 1))) {
              nonzero_neighbours_cnt += (block[neighbour] != 0);
            }
          }

          if (diag < threshold) {
            encoder.encodeBit(significant_coef_flag_high_ctx[nonzero_neighbours_cnt], coef);
          }
          else {
            encoder.encodeBit(significant_coef_flag_low_ctx[nonzero_neighbours_cnt], coef);
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

            encoder.encodeBit(coef_greater_one_ctx[diag], coef > 1);

            if (coef > 1) {
              encoder.encodeBit(coef_greater_two_ctx[diag], coef > 2);

              if (coef > 2) {
                encoder.encodeU(coef_abs_level_ctx[diag], coef - 3);
              }
            }

            encoder.encodeBitBypass(sign);
          }
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

  void decodeBlock(CABACDecoder &decoder, DynamicBlock<float, D> &block) {
    std::vector<bool> nonzero_diags(diagonals);

    for (size_t diag = 0; diag < diagonals; diag++) {
      nonzero_diags[diag] = decoder.decodeBit(coded_diag_flag_ctx[diag]);

      if (nonzero_diags[diag]) {
        if (decoder.decodeBit(last_coded_diag_flag_ctx[diag])) {
          break;
        }
      }
    }

    for (size_t d = 1; d <= diagonals; d++) {
      size_t diag = diagonals - d;
      int64_t zero_coef_distr = 0;

      if (nonzero_diags[diag]) {
        for (auto &i: scan_table[diag]) {
          int64_t coef {};

          size_t nonzero_neighbours_cnt = 0;

          for (size_t dim = 0; dim < D; dim++) {
            size_t neighbour = i + block.stride(dim);
            if ((i % block.stride(dim + 1)) < (neighbour % block.stride(dim + 1))) {
              nonzero_neighbours_cnt += (block[neighbour] != 0);
            }
          }

          if (diag < threshold) {
            coef = decoder.decodeBit(significant_coef_flag_high_ctx[nonzero_neighbours_cnt]);
          }
          else {
            coef = decoder.decodeBit(significant_coef_flag_low_ctx[nonzero_neighbours_cnt]);
          }

          if (!coef) {
            zero_coef_distr++;
          }
          else {
            zero_coef_distr--;

            coef += decoder.decodeBit(coef_greater_one_ctx[diag]);

            if (coef > 1) {
              coef += decoder.decodeBit(coef_greater_two_ctx[diag]);

              if (coef > 2) {
                coef += decoder.decodeU(coef_abs_level_ctx[diag]);
              }
            }

            if (decoder.decodeBitBypass()) {
              coef = -coef;
            }
          }

          block[i] = coef << discarded_bits;
        }
      }

      if ((zero_coef_distr > 0) && (threshold > diag)) {
        threshold--;
      }
      else if ((zero_coef_distr < 0) && (threshold < diag)) {
        threshold++;
      }
    }

    auto proxy = [&](size_t index) -> auto & {
      return block[index];
    };

    idct<D>(block.size(), proxy);
  }
};
