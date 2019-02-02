/******************************************************************************\
* SOUBOR: qtable.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef QTABLE_H
#define QTABLE_H

template<size_t D>
inline constexpr QuantTable<D> baseQuantTableLuma() {
  constexpr QuantTable<2> base_luma {
    16, 11, 10, 16, 124, 140, 151, 161,
    12, 12, 14, 19, 126, 158, 160, 155,
    14, 13, 16, 24, 140, 157, 169, 156,
    14, 17, 22, 29, 151, 187, 180, 162,
    18, 22, 37, 56, 168, 109, 103, 177,
    24, 35, 55, 64, 181, 104, 113, 192,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 199,
  };

  QuantTable<D> quant_table {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    quant_table[i] = base_luma[i%64];
  }

  return quant_table;
}

template<size_t D>
inline constexpr QuantTable<D> baseQuantTableChroma() {
  constexpr QuantTable<2> base_chroma {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
  };

  QuantTable<D> quant_table {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    quant_table[i] = base_chroma[i%64];
  }

  return quant_table;
}

#endif
