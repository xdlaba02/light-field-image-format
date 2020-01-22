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
#include <cmath>

#include <array>
#include <vector>
#include <fstream>
#include <iterator>

using RGBUNIT   = uint16_t; /**< @brief Unit which is intended to containt RGB data.*/
using QDATAUNIT = int64_t;  /**< @brief Unit which is intended to containt quantized DCT coefficients.*/
using INPUTUNIT = float;    /**< @brief Unit which is intended to containt input/output YCbCr/YCoCg/other data.*/

using INPUTTRIPLET = std::array<INPUTUNIT, 3>; /**< @brief Unit which is intended to containt one triplet of input data.*/

/**
* @brief The block type. Basically an array parametrized by type, size and number of dimensions.
*/
template<typename T, size_t BS, size_t D>
using Block = std::array<T, static_cast<size_t>(constpow(BS, D))>;

template<typename T, size_t D>
class DynamicBlock {
  std::array<size_t, D> m_size {};
  std::vector<T>        m_data {};

public:
  DynamicBlock() = default;

  DynamicBlock(size_t BS): m_data(pow(BS, D)) {
    std::fill(std::begin(m_size), std::end(m_size), BS);
  }

  DynamicBlock(const size_t size[D]): m_size{}, m_data(get_stride<D>(size)) {
    std::copy(size, size + D, std::begin(m_size));
  }

  DynamicBlock(const std::array<size_t, D> &size): DynamicBlock(size.data()) {}

  void resize(const std::array<size_t, D> &size) {
    m_size = size;
    m_data.resize(get_stride<D>(size));
  }

  T &operator[](size_t index) {
    return m_data[index];
  }

  const T &operator[](size_t index) const {
    return m_data[index];
  }

  T &operator[](const std::array<size_t, D> &pos) {
    size_t index {};
    for (size_t i { 1 }; i <= D; i++) {
      index *= m_size[D - i];
      index += pos[D - i];
    }
    return m_data[index];
  }

  const T &operator[](const std::array<size_t, D> &pos) const {
    size_t index {};
    for (size_t i { 1 }; i <= D; i++) {
      index *= m_size[D - i];
      index += pos[D - i];
    }
    return m_data[index];
  }

  const std::array<size_t, D> &size() const {
    return m_size;
  }

  size_t size(size_t i) const {
    return m_size[i];
  }

  size_t stride(size_t depth) const {
    if (depth == 0) {
      return 1;
    }
    else {
      return m_size[depth - 1] * stride(depth - 1);
    }
  }

  void fill(T value) {
    std::fill(std::begin(m_data), std::end(m_data), value);
  }

};

#endif
