/**
* @file lfif_encoder.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for encoding an image.
*/

#pragma once

#include "block_operations.h"
#include "contexts.h"
#include "bitstream.h"
#include "predict.h"
#include "stack_allocator.h"
#include "lfif.h"

#include <cstdint>

#include <ostream>
#include <sstream>
#include <map>

/**
* @brief Function which encodes the image to the stream.
* @param enc The encoder structure.
* @param input Callback function specified by client with signature T input(size_t index), where T is pixel/sample type.
* @param output Output stream to which the image shall be encoded.
*/
template<size_t D, typename IF, typename OF>
void encodeStreamDCT(std::ostream &output, const LFIF<D> &image, IF &&puller, OF &&pusher) {
  StackAllocator::init(2147483648); //FIXME

  DynamicBlock<float,   D> current_block[3] {image.block_size, image.block_size, image.block_size};
  DynamicBlock<int64_t, D> quantized_block(image.block_size);
  DynamicBlock<float,   D> prediction_block(image.block_size);

  std::array<size_t, D> aligned_image_size {};
  for (size_t i = 0; i < D; i++) {
    aligned_image_size[i] = ((image.size[i] + image.block_size[i] - 1) / image.block_size[i]) * image.block_size[i];
  }

  std::array<ThresholdedDiagonalContexts<D>, 2> contexts {
    ThresholdedDiagonalContexts<D>(image.block_size),
    ThresholdedDiagonalContexts<D>(image.block_size)
  };

  PredictionModeContexts<D>        prediction_ctx {};
  std::vector<std::vector<size_t>> scan_table     {};
  OBitstream                       bitstream      {};
  CABACEncoder                     cabac          {};

  // init scan table
  scan_table.resize(num_diagonals<D>(image.block_size));
  iterate_dimensions<D>(image.block_size, [&](const auto &pos) {
    size_t diagonal {};
    for (size_t i = 0; i < D; i++) {
      diagonal += pos[i];
    }

    scan_table[diagonal].push_back(make_index(image.block_size, pos));
  });


  bitstream.open(output);
  cabac.init(bitstream);

  block_for<D>({}, image.block_size, aligned_image_size, [&](const std::array<size_t, D> &offset) {
    for (size_t i = 0; i < D; i++) {
      std::cerr << offset[i] << " ";
    }
    std::cerr << " out of ";
    for (size_t i { 0 }; i < D; i++) {
      std::cerr << aligned_image_size[i] << " ";
    }
    std::cerr << "\n";

    moveBlock<D>(puller, image.size, offset,
                 [&](const auto &block_pos, const auto &value) {
                   current_block[0][block_pos] = value[0];
                   current_block[1][block_pos] = value[1];
                   current_block[2][block_pos] = value[2];
                 }, image.block_size, {},
                 image.block_size);

    auto predInputF = [&](std::array<int64_t, D> &block_pos, size_t channel) -> float {
      if (offset == std::array<size_t, D>{}) {
        return 0.f;
      }

      //resi pokud se prediktor chce divat na vzorky, ktere jeste nebyly komprimovane
      for (size_t i = 1; i <= D; i++) {
        size_t idx = D - i;

        if (block_pos[idx] < 0) {
          if (offset[idx] > 0) {
            break;
          }
        }
        else if (block_pos[idx] >= static_cast<int64_t>(image.block_size[idx])) {
          block_pos[idx] = image.block_size[idx] - 1;
        }
      }

      //resi pokud se prediktor chce divat na vzorky mimo obrazek v zapornych souradnicich
      int64_t min_pos = std::numeric_limits<int64_t>::max();
      for (size_t i = 0; i < D; i++) {
        if (offset[i] > 0) {
          if (block_pos[i] < min_pos) {
            min_pos = block_pos[i];
          }
        }
        else if (block_pos[i] < 0) {
          block_pos[i] = 0;
        }
      }

      for (size_t i = 0; i < D; i++) {
        if (offset[i] > 0) {
          block_pos[i] -= min_pos + 1;
        }
      }

      std::array<size_t, D> image_pos {};
      for (size_t i = 0; i < D; i++) {
        //resi pokud se prediktor chce divat na vzorky mimo obrazek v kladnych souradnicich
        image_pos[i] = std::clamp<size_t>(offset[i] + block_pos[i], 0, image.size[i] - 1);
      }

      return puller(image_pos)[channel];
    };

    uint64_t prediction_type {};
    if (image.predicted) {
      prediction_type = find_best_prediction_type<D>(current_block[0], [&](std::array<int64_t, D> &block_pos) { return predInputF(block_pos, 0); });
      encodePredictionType<D>(prediction_type, cabac, prediction_ctx);
    }

    for (size_t channel = 0; channel < 3; channel++) {
      if (image.predicted) {
        predict<D>(prediction_block, prediction_type, [&](std::array<int64_t, D> &block_pos) { return predInputF(block_pos, channel); });
        applyPrediction<D>(current_block[channel], prediction_block);
      }

      forwardDiscreteCosineTransform<D>(current_block[channel]);

      quantize<D>(current_block[channel], quantized_block, image.discarded_bits);

      encodeBlock<D>(quantized_block, cabac, contexts[channel != 0], scan_table);

      if (image.predicted) {
        dequantize<D>(quantized_block, current_block[channel], image.discarded_bits);
        inverseDiscreteCosineTransform<D>(current_block[channel]);
        disusePrediction<D>(current_block[channel], prediction_block);
      }
    }

    if (image.predicted) {
      moveBlock<D>([&](const auto &pos) {
        return std::array<float, 3> { current_block[0][pos], current_block[1][pos], current_block[2][pos] };
      }, image.block_size, {},
      pusher, image.size, offset,
      image.block_size);
    }
  });

  cabac.terminate();
  bitstream.flush();

  StackAllocator::cleanup();
}




template<size_t D, typename IF, typename OF>
void encodeStreamDWT(std::ostream &output, const LFIF<D> &image, IF &&puller, OF &&pusher) {
  StackAllocator::init(2147483648); //FIXME

  DynamicBlock<int32_t, D> current_block[3] {image.block_size, image.block_size, image.block_size};
  DynamicBlock<int64_t, D> quantized_block(image.block_size);

  std::array<size_t, D> aligned_image_size {};
  for (size_t i = 0; i < D; i++) {
    aligned_image_size[i] = ((image.size[i] + image.block_size[i] - 1) / image.block_size[i]) * image.block_size[i];
  }

  std::array<ThresholdedDiagonalContexts<D>, 2> contexts {
    ThresholdedDiagonalContexts<D>(image.block_size),
    ThresholdedDiagonalContexts<D>(image.block_size)
  };

  std::vector<std::vector<size_t>> scan_table     {};
  OBitstream                       bitstream      {};
  CABACEncoder                     cabac          {};

  // init scan table
  scan_table.resize(num_diagonals<D>(image.block_size));
  iterate_dimensions<D>(image.block_size, [&](const auto &pos) {
    size_t diagonal {};
    for (size_t i = 0; i < D; i++) {
      diagonal += pos[i];
    }

    scan_table[diagonal].push_back(make_index(image.block_size, pos));
  });


  bitstream.open(output);
  cabac.init(bitstream);

  block_for<D>({}, image.block_size, aligned_image_size, [&](const std::array<size_t, D> &offset) {
    for (size_t i = 0; i < D; i++) {
      std::cerr << offset[i] << " ";
    }
    std::cerr << " out of ";
    for (size_t i { 0 }; i < D; i++) {
      std::cerr << aligned_image_size[i] << " ";
    }
    std::cerr << "\n";

    moveBlock<D>(puller, image.size, offset,
                 [&](const auto &block_pos, const auto &value) {
                   current_block[0][block_pos] = value[0];
                   current_block[1][block_pos] = value[1];
                   current_block[2][block_pos] = value[2];
                 }, image.block_size, {},
                 image.block_size);

    for (size_t channel = 0; channel < 3; channel++) {
      forwardDiscreteWaveletTransform<D>(current_block[channel]);

      for (size_t i = 0; i < get_stride<D>(image.block_size); i++) {
        quantized_block[i] = std::round(ldexp(current_block[channel][i], -image.discarded_bits));
      }

      encodeBlock<D>(quantized_block, cabac, contexts[channel != 0], scan_table);
    }
  });

  cabac.terminate();
  bitstream.flush();

  StackAllocator::cleanup();
}
