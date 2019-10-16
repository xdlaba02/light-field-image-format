/**
* @file lfiftypes.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Some basic types used by the library.
*/

#ifndef LFIFTYPES_H
#define LFIFTYPES_H

#include "meta.h"

#include <cstdint>

#include <array>
#include <fstream>

using RGBUNIT   = uint16_t; /**< @brief Unit which is intended to containt RGB data.*/
using QDATAUNIT = int64_t;  /**< @brief Unit which is intended to containt quantized DCT coefficients.*/
using INPUTUNIT = float;    /**< @brief Unit which is intended to containt input/output YCbCr/YCoCg/other data.*/

using INPUTTRIPLET = std::array<INPUTUNIT, 3>; /**< @brief Unit which is intended to containt one triplet of input data.*/

/**
* @brief The block type. Basically an array parametrized by type, size and number of dimensions.
*/
template<typename T, size_t BS, size_t D>
using Block = std::array<T, static_cast<size_t>(constpow(BS, D))>;

#endif
