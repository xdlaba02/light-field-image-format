#pragma once

#include "components/block.h"
#include "components/cabac.h"
#include "components/colorspace.h"
#include "components/meta.h"
#include "components/dct.h"
#include "components/diag_scan.h"

#include "lfif.h"

#include <cstdint>
#include <cstddef>

#include <array>

template<size_t D>
class DCTBlockTransformer {
  DCTCoefs<D> dct_coefs;
  std::array<size_t, D> block_size;
  uint8_t discarded_bits;

public:

  DCTBlockTransformer(const std::array<size_t, D> &block_size, uint8_t discarded_bits): dct_coefs(block_size.data()) {
    this->block_size = block_size;
    this->discarded_bits = discarded_bits;
  }

  void forwardPass(DynamicBlock<float, D> &block) {
    auto proxy = [&](size_t index) -> auto & {
      return block[index];
    };

    fdct<D>(this->block_size, dct_coefs, proxy);

    iterate_dimensions<D>(this->block_size, [&](const auto &pos) {
      block[pos] = std::round(ldexp(block[pos], -this->discarded_bits)); // QUANTIZATION
    });
  }

  void inversePass(DynamicBlock<float, D> &block) {
    iterate_dimensions<D>(this->block_size, [&](const auto &pos) {
      block[pos] = ldexp(block[pos], this->discarded_bits);
    });

    auto proxy = [&](size_t index) -> auto & {
      return block[index];
    };

    idct<D>(block.size(), dct_coefs, proxy);
  }
};
