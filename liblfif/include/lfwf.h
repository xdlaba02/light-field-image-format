#pragma once

#include <cstdint>

#include <array>

template <size_t D>
struct LFWF {
  std::array<size_t, D> size;
  std::array<size_t, D> block_size;
  uint8_t depth_bits;
  uint8_t discarded_bits;
  bool predicted;
};
