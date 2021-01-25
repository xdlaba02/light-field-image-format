#pragma once

#include "components/block.h"
#include "components/cabac.h"
#include "components/predict.h"

#include <cstddef>
#include <cstdint>

template <size_t D>
class BlockPredictor {
  template <class T>
  static inline constexpr T pow(const T base, const uint64_t exponent) {
      return (exponent == 0) ? 1 : (base * pow(base, exponent - 1));
  }

  CABAC::ContextModel dc_prediction_ctx;
  CABAC::ContextModel planar_prediction_ctx;
  CABAC::ContextModel direction_prediction_ctx;
  std::array<std::array<CABAC::ContextModel, 5>, D> directions_prediction_ctx;

  DynamicBlock<float, D> decoded_image;
  std::array<size_t, D> image_size;

public:
  struct PredictionType {
    uint64_t type;
    std::array<int8_t, D> direction;
  };

private:
  void predict(DynamicBlock<float, D> &block, const std::array<size_t, D> &offset, PredictionType type) {
    auto inputF = [&](std::array<int64_t, D> block_pos) -> float {
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
        else if (block_pos[idx] >= static_cast<int64_t>(block.size(idx))) {
          block_pos[idx] = block.size(idx) - 1;
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
        image_pos[i] = std::clamp<size_t>(offset[i] + block_pos[i], 0, image_size[i] - 1);
      }

      return decoded_image[image_pos];
    };

    if (type.type == 0) {
      for (size_t i = 0; i < block.stride(D); i++) {
        block[i] = 0.f;
      }
    }
    if (type.type == 1) {
      int64_t value = predict_DC<D>(block.size(), inputF);
      for (size_t i = 0; i < block.stride(D); i++) {
        block[i] = value;
      }
    }
    else if (type.type == 2) {
      predict_planar<D>(block, inputF);
    }
    else if (type.type == 3) {
      predict_direction<D>(block, type.direction.data(), inputF);
    }
  }

public:

  BlockPredictor(const std::array<size_t, D> &image_size): decoded_image(image_size), image_size(image_size) {};

  void encodePredictionType(const PredictionType &type, CABACEncoder &encoder) {
    encoder.encodeBit(dc_prediction_ctx, type.type == 1);
    if (type.type > 1) {
      encoder.encodeBit(planar_prediction_ctx, type.type == 2);
      if (type.type > 2) {
        encoder.encodeBit(direction_prediction_ctx, type.type == 3);
        if (type.type == 3) {
          for (size_t i = 0; i < D; i++) {
            encoder.encodeBit(directions_prediction_ctx[i][0], type.direction[i] == 0);
            if (type.direction[i]) {
              encoder.encodeBit(directions_prediction_ctx[i][1], std::abs(type.direction[i]) == 1);
              encoder.encodeBit(directions_prediction_ctx[i][2], type.direction[i] < 0);
            }
          }
        }
      }
    }
  }

  PredictionType decodePredictionType(CABACDecoder &decoder) {
    PredictionType type {};

    if (decoder.decodeBit(dc_prediction_ctx)) {
      type.type = 1;
    }
    else if (decoder.decodeBit(planar_prediction_ctx)) {
      type.type = 2;
    }
    else if (decoder.decodeBit(direction_prediction_ctx)) {
      type.type = 3;

      for (size_t i = 0; i < D; i++) {
        if (decoder.decodeBit(directions_prediction_ctx[i][0])) {
          type.direction[i] = decoder.decodeBit(directions_prediction_ctx[i][1]) ? 1 : 2;
          if (decoder.decodeBit(directions_prediction_ctx[i][2])) {
            type.direction[i] = -type.direction[i];
          }
        }
      }
    }

    return type;
  }

  PredictionType selectPredictionType(const DynamicBlock<float, D> &input_block, const std::array<size_t, D> &offset) {
    DynamicBlock<float, D> prediction_block(input_block.size());

    auto prediction_error = [&]() -> auto {
      float error = 0.f;

      iterate_dimensions<D>(input_block.size(), [&](const auto &pos) {
        error += (input_block[pos] - prediction_block[pos]) * (input_block[pos] - prediction_block[pos]);
      });

      return error;
    };

    PredictionType best_prediction_type {};
    float lowest_error = prediction_error(); // No prediction, just input block energy.

    auto eval_prediction = [&](PredictionType type) {
      predict(prediction_block, offset, type);

      float current_error = prediction_error();
      if (current_error < lowest_error) {
        lowest_error = current_error;
        best_prediction_type = type;
      }
    };

    eval_prediction({1, {}});
    eval_prediction({2, {}});

    iterate_cube<5, D>([&](const std::array<size_t, D> &pos) {
      std::array<int8_t, D> direction {};

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

      eval_prediction({3, direction});
    });

    return best_prediction_type;
  }

  void forwardPass(DynamicBlock<float, D> &block, const std::array<size_t, D> &offset, PredictionType type) {
    DynamicBlock<float, D> prediction_block(block.size());

    predict(prediction_block, offset, type);

    iterate_dimensions<D>(block.size(), [&](const auto &pos) {
      block[pos] -= prediction_block[pos];
    });
  }

  void backwardPass(DynamicBlock<float, D> &block, const std::array<size_t, D> &offset, PredictionType type) {
    DynamicBlock<float, D> prediction_block(block.size());

    predict(prediction_block, offset, type);

    iterate_dimensions<D>(block.size(), [&](const auto &pos) {
      block[pos] += prediction_block[pos];
    });

    moveBlock<D>([&](const auto &pos) { return block[pos]; }, block.size(), {},
                 [&](const auto &pos, const auto &val) { return decoded_image[pos] = val; }, image_size, offset,
                 block.size());
  }
};
