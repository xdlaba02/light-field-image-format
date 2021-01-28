#pragma once

#include "components/block.h"
#include "components/dwt.h"

#include <cstdint>
#include <cstddef>

#include <array>

template<size_t D>
class DWTBlockTransformer {
  std::array<size_t, D> block_size;
  uint8_t discarded_bits;

public:

  DWTBlockTransformer(const std::array<size_t, D> &block_size, uint8_t discarded_bits) {
    this->block_size = block_size;
    this->discarded_bits = discarded_bits;
  }

  void forwardPass(DynamicBlock<int32_t, D> &block) {
    auto proxy = [&](size_t index) -> auto & {
      return block[index];
    };

    fdwt<D>(this->block_size, proxy);

    iterate_dimensions<D>(this->block_size, [&](const auto &pos) {
      block[pos] >>= this->discarded_bits; //QUANTIZATION
    });
  }

  void inversePass(DynamicBlock<int32_t, D> &block) {
    iterate_dimensions<D>(this->block_size, [&](const auto &pos) {
      block[pos] <<= this->discarded_bits;
    });

    auto proxy = [&](size_t index) -> auto & {
      return block[index];
    };

    idwt<D>(this->block_size, proxy);
  }
};
