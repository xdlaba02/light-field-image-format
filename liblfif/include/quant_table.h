/******************************************************************************\
* SOUBOR: quant_table.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef QUANT_TABLE_H
#define QUANT_TABLE_H

#include "lfiftypes.h"
#include "dct.h"

#include <iosfwd>

using QTABLEUNIT = uint64_t;

template <size_t D>
class QuantTable {
public:
  QuantTable<D> &baseDiagonalTable(const std::array<QTABLEUNIT, 64> &qtable, uint8_t quality);
  QuantTable<D> &baseCopyTable(const std::array<QTABLEUNIT, 64> &qtable, uint8_t quality);
  QuantTable<D> &baseUniform(uint8_t quality);

  QuantTable<D> &writeToStream(std::ofstream &stream);
  QuantTable<D> &readFromStream(std::ifstream &stream);

  QTABLEUNIT operator [](size_t index) const;

  static constexpr std::array<QTABLEUNIT, 64> base_luma = {
    16,  11,  10,  16,  24,  40,  51,  61,
    12,  12,  14,  19,  26,  58,  60,  55,
    14,  13,  16,  24,  40,  57,  69,  56,
    14,  17,  22,  29,  51,  87,  80,  62,
    18,  22,  37,  56,  68, 109, 103,  77,
    24,  35,  55,  64,  81, 104, 113,  92,
    49,  64,  78,  87, 103, 121, 120, 101,
    72,  92,  95,  98, 112, 100, 103,  99
  };

  static constexpr std::array<QTABLEUNIT, 64> base_chroma = {
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
  QuantTable<D> &scaleByQuality(uint8_t quality);

  Block<QTABLEUNIT, D> m_block;
};

#endif
