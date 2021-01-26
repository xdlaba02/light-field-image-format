#pragma once

#include "components/endian.h"


#include <cstdint>

#include <array>
#include <istream>
#include <ostream>


template <size_t D>
struct LFIF {
  std::array<size_t, D> size;
  std::array<size_t, D> block_size;
  uint8_t depth_bits;
  uint8_t discarded_bits;
  bool predicted;
};
