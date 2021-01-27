#pragma once

#include <cstddef>
#include <cstdint>

#include <array>

template <size_t D>
struct PredictionType {
  uint8_t type;
  std::array<int8_t, D> direction;
};
