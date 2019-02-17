/******************************************************************************\
* SOUBOR: lfiftypes.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIFTYPES_H
#define LFIFTYPES_H

#include "constpow.h"

#include <cstdint>

#include <array>

using RGBUnit8           = uint8_t;
using RGBUnit16          = uint16_t;

using YCbCrUnit8          = double;
using YCbCrUnit16         = double;

using QuantizedDataUnit8  = int16_t;
using QuantizedDataUnit16 = int32_t;

template<typename T>
struct RGBPixel {
  T r;
  T g;
  T b;
};

template<typename T, size_t D>
using Block = std::array<T, static_cast<size_t>(constpow(8, D))>;

#endif
