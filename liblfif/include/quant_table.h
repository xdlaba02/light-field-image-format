/******************************************************************************\
* SOUBOR: quant_table.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef QUANT_TABLE_H
#define QUANT_TABLE_H

#include "lfiftypes.h"
#include "dct.h"
#include "endian_t.h"

#include <iosfwd>

using QTABLEUNIT = uint64_t;

template <size_t BS, size_t D>
class QuantTable {
public:
  QuantTable<BS, D> &baseDiagonalTable(const Block<QTABLEUNIT, 8, 2> &qtable, uint8_t quality);
  QuantTable<BS, D> &baseCopyTable(const Block<QTABLEUNIT, 8, 2> &qtable, uint8_t quality);
  QuantTable<BS, D> &baseUniform(uint8_t quality);

  const QuantTable<BS, D> &writeToStream(std::ostream &stream) const;
  QuantTable<BS, D> &readFromStream(std::istream &stream);

  QTABLEUNIT operator [](size_t index) const;

  static constexpr Block<QTABLEUNIT, 8, 2> base_luma = {
    16,  11,  10,  16,  24,  40,  51,  61,
    12,  12,  14,  19,  26,  58,  60,  55,
    14,  13,  16,  24,  40,  57,  69,  56,
    14,  17,  22,  29,  51,  87,  80,  62,
    18,  22,  37,  56,  68, 109, 103,  77,
    24,  35,  55,  64,  81, 104, 113,  92,
    49,  64,  78,  87, 103, 121, 120, 101,
    72,  92,  95,  98, 112, 100, 103,  99
  };

  static constexpr Block<QTABLEUNIT, 8, 2> base_chroma = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
  };

private:
  QTABLEUNIT averageForDiagonal(const std::array<QTABLEUNIT, 64> &qtable, size_t diagonal);
  QuantTable<BS, D> &scaleByQuality(uint8_t quality);

  Block<QTABLEUNIT, BS, D> m_block;
};

template <size_t BS, size_t D>
QuantTable<BS, D> &
QuantTable<BS, D>::baseDiagonalTable(const std::array<QTABLEUNIT, 64> &qtable, uint8_t quality) {
  for (size_t i = 0; i < constpow(BS, D); i++) {
    size_t diagonal = 0;
    for (size_t j = 0; j < D; j++) {
      diagonal += (i % constpow(BS, j + 1)) / constpow(BS, j);
    }
    m_block[i] = averageForDiagonal(qtable, diagonal);
  }

  scaleByQuality(quality);

  return *this;
}

template <size_t BS, size_t D>
QuantTable<BS, D> &
QuantTable<BS, D>::baseCopyTable(const std::array<QTABLEUNIT, 64> &qtable, uint8_t quality) {
  for (size_t slice = 0; slice < constpow(BS, D - 2); slice++) {
    for (size_t y = 0; y < BS; y++) {
      size_t orig_y = y < 8 ? y : 7;

      for (size_t x = 0; x < BS; x++) {
        size_t orig_x = x < 8 ? x : 7;

        m_block[(slice * BS + y) * BS + x] = qtable[orig_y * 8 + orig_x];
      }
    }
  }

  scaleByQuality(quality);

  return *this;
}

template <size_t BS, size_t D>
QuantTable<BS, D> &
QuantTable<BS, D>::baseUniform(uint8_t quality) {
  for (size_t i = 0; i < constpow(BS, D); i++) {
    m_block[i] = 101 - quality;
  }

  return *this;
}

template <size_t BS, size_t D>
const QuantTable<BS, D> &QuantTable<BS, D>::writeToStream(std::ostream &stream) const {
  for (size_t i = 0; i < constpow(BS, D); i++) {
    writeValueToStream<uint8_t>(m_block[i], stream);
  }

  return *this;
}

template <size_t BS, size_t D>
QuantTable<BS, D> &
QuantTable<BS, D>::readFromStream(std::istream &stream) {
  for (size_t i = 0; i < constpow(BS, D); i++) {
    m_block[i] = readValueFromStream<uint8_t>(stream);
  }
  return *this;
}

template <size_t BS, size_t D>
uint64_t QuantTable<BS, D>::operator [](size_t index) const {
  return m_block[index];
}

template <size_t BS, size_t D>
QTABLEUNIT QuantTable<BS, D>::averageForDiagonal(const std::array<QTABLEUNIT, 64> &qtable, size_t diagonal) {
  double sum = 0;
  size_t cnt = 0;

  for (size_t y = 0; y < 8; y++) {
    for (size_t x = 0; x < 8; x++) {
      if (x + y == diagonal) {
        sum += qtable[y*8+x];
        cnt++;
      }
    }
  }

  if (cnt) {
    return (sum / cnt) + 0.5;
  }

  return qtable[63];
}

template <size_t BS, size_t D>
QuantTable<BS, D> &
QuantTable<BS, D>::scaleByQuality(uint8_t quality) {
  double scale_coef = quality < 50 ? (50.0 / quality) : (200.0 - 2 * quality) / 100;

  for (size_t i = 0; i < constpow(BS, D); i++) {
    m_block[i] = std::clamp<double>(m_block[i] * scale_coef, 1, 255);
  }

  return *this;
}

#endif
