#pragma once

#include "endian_t.h"

#include <cstdint>
#include <cstddef>

#include <iostream>
#include <sstream>
#include <array>

template <size_t D>
struct LFIF {
  std::array<size_t, D> size;
  std::array<size_t, D> block_size;
  uint8_t depth_bits;
  uint8_t discarded_bits;
  bool predicted;

  void open(std::istream &input) {
    depth_bits     = readValueFromStream<uint8_t>(input);
    discarded_bits = readValueFromStream<uint8_t>(input);
    predicted      = readValueFromStream<bool>(input);

    for (size_t i = 0; i < D; i++) {
      size[i] = readValueFromStream<uint64_t>(input);
    }

    for (size_t i = 0; i < D; i++) {
      block_size[i] = readValueFromStream<uint64_t>(input);
    }
  }

  void create(
            std::ostream          &output,
      const std::array<size_t, D> &size,
      const std::array<size_t, D> &block_size,
            uint8_t                depth_bits,
            uint8_t                discarded_bits,
            bool                   predicted) {

    this->size = size;
    this->block_size = block_size;
    this->depth_bits = depth_bits;
    this->discarded_bits = discarded_bits;
    this->predicted = predicted;

    writeValueToStream<uint8_t>(depth_bits, output);
    writeValueToStream<uint8_t>(discarded_bits, output);
    writeValueToStream<bool>(predicted, output);

    for (size_t i = 0; i < D; i++) {
      writeValueToStream<uint64_t>(size[i], output);
    }

    for (size_t i = 0; i < D; i++) {
      writeValueToStream<uint64_t>(block_size[i], output);
    }
  }
};
