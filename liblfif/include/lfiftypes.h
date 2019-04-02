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

using RGBUNIT   = uint16_t;
using QDATAUNIT = int64_t;
using INPUTUNIT = float;

template<typename T, size_t BS, size_t D>
using Block = std::array<T, static_cast<size_t>(constpow(BS, D))>;

#endif
