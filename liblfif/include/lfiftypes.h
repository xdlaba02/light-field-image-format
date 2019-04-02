/******************************************************************************\
* SOUBOR: lfiftypes.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIFTYPES_H
#define LFIFTYPES_H

#include "constpow.h"

#include <cstdint>

#include <array>
#include <fstream>

#ifdef M_BLOCK_SIZE
const size_t BLOCK_SIZE = M_BLOCK_SIZE;
#else
const size_t BLOCK_SIZE = 8;
#endif

using RGBUNIT   = uint16_t;
using QDATAUNIT = int64_t;
using YCBCRUNIT = float;

template<typename T, size_t D>
using Block = std::array<T, static_cast<size_t>(constpow(BLOCK_SIZE, D))>;

#endif
