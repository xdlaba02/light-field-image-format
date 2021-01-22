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

  struct ThresholdedDiagonalContexts {
    size_t threshold;

    std::vector<CABAC::ContextModel> significant_coef_flag_high_ctx;
    std::vector<CABAC::ContextModel> significant_coef_flag_low_ctx;

    std::vector<CABAC::ContextModel> coded_diag_flag_ctx;
    std::vector<CABAC::ContextModel> last_coded_diag_flag_ctx;
    std::vector<CABAC::ContextModel> coef_greater_one_ctx;
    std::vector<CABAC::ContextModel> coef_greater_two_ctx;
    std::vector<CABAC::ContextModel> coef_abs_level_ctx;

    void init(const std::array<size_t, D> &block_size) {
      size_t diagonals = num_diagonals<D>(block_size);

      threshold = diagonals / 2;

      coded_diag_flag_ctx.resize(diagonals);
      last_coded_diag_flag_ctx.resize(diagonals);
      significant_coef_flag_high_ctx.resize(D + 1);
      significant_coef_flag_low_ctx.resize(D + 1);
      coef_greater_one_ctx.resize(diagonals);
      coef_greater_two_ctx.resize(diagonals);
      coef_abs_level_ctx.resize(diagonals);
    }
  };

  std::array<size_t, D> aligned_image_size;

  std::array<ThresholdedDiagonalContexts, 2> contexts;
  std::vector<std::vector<size_t>> scan_table;

  OBitstream   bitstream;
  CABACEncoder cabac;

  const LFIF<D> *image;

  static void forwardDiscreteCosineTransform(DynamicBlock<float, D> &block) {
    auto proxy = [&](size_t index) -> auto & {
      return block[index];
    };

    fdct<D>(block.size(), proxy);
  }

  static void quantize(const DynamicBlock<float, D> &input, DynamicBlock<int32_t, D> &output, uint8_t shift_val) {
    for (size_t i = 0; i < get_stride<D>(input.size()); i++) {
      output[i] = std::round(ldexp(input[i], -shift_val));
    }
  }

  static void encodeBlock(const DynamicBlock<int32_t, D> &block, CABACEncoder &encoder, ThresholdedDiagonalContexts &contexts, const std::vector<std::vector<size_t>> &scan_table) {
    const size_t diagonals = num_diagonals<D>(block.size());

    std::vector<bool> nonzero_diags(diagonals);
    size_t diags_cnt { 0 };

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

    for (size_t d = 1; d <= diagonals; d++) {
      size_t diag = diagonals - d;
      int32_t zero_coef_distr { 0 };

      if (nonzero_diags[diag]) {
        for (auto &i: scan_table[diag]) {
          int32_t coef = block[i];

          size_t nonzero_neighbours_cnt { 0 };

          for (size_t dim = 0; dim < D; dim++) {
            size_t neighbour = i + block.stride(dim);
            if ((i % block.stride(dim + 1)) < (neighbour % block.stride(dim + 1))) {
              nonzero_neighbours_cnt += (block[neighbour] != 0);
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


public:
  BlockEncoderDCT(const LFIF<D> &img) {
    StackAllocator::init(2147483648); //FIXME
    image = &img;

    for (size_t i = 0; i < D; i++) {
      aligned_image_size[i] = ((image->size[i] + image->block_size[i] - 1) / image->block_size[i]) * image->block_size[i];
    }

    scan_table.resize(num_diagonals<D>(image->block_size));
    iterate_dimensions<D>(image->block_size, [&](const auto &pos) {
      size_t diagonal {};
      for (size_t i = 0; i < D; i++) {
        diagonal += pos[i];
      }

      scan_table[diagonal].push_back(make_index(image->block_size, pos));
    });

    contexts[0].init(image->block_size);
    contexts[1].init(image->block_size);

  }

  ~BlockEncoderDCT() {
    StackAllocator::cleanup();
  }

  template <typename IF>
  void encodeStream(IF &&puller, std::ostream &output) {
    bitstream.open(output);
    cabac.init(bitstream);

    block_for<D>({}, image->block_size, aligned_image_size, [&](const std::array<size_t, D> &offset) {
      DynamicBlock<float,   D> floating_blocks[3] {image->block_size, image->block_size, image->block_size};
      DynamicBlock<int32_t, D> quantized_block(image->block_size);

      moveBlock<D>(puller, image->size, offset,
                   [&](const auto &block_pos, const auto &value) {
                     floating_blocks[0][block_pos] = YCbCr::RGBToY(value[0], value[1], value[2]) - pow(2, image->depth_bits - 1);
                     floating_blocks[1][block_pos] = YCbCr::RGBToCb(value[0], value[1], value[2]);
                     floating_blocks[2][block_pos] = YCbCr::RGBToCr(value[0], value[1], value[2]);
                   }, image->block_size, {},
                   image->block_size);

      for (size_t ch = 0; ch < 3; ch++) {
        forwardDiscreteCosineTransform(floating_blocks[ch]);
        quantize(floating_blocks[ch], quantized_block, image->discarded_bits);
        encodeBlock(quantized_block, cabac, contexts[ch != 0], scan_table);
      }
    });

    cabac.terminate();
    bitstream.flush();
  }
};
