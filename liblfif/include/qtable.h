/******************************************************************************\
* SOUBOR: qtable.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef QTABLE_H
#define QTABLE_H

#include <algorithm>
#include <fstream>

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

template<size_t D>
inline QuantTable<D> scaleQuantTable(QuantTable<D> quant_table, const uint8_t quality) {
  float scale_coef = quality < 50 ? (5000.0 / quality) / 100 : (200.0 - 2 * quality) / 100;

  for (size_t i = 0; i < constpow(8, D); i++) {
    quant_table[i] = clamp<float>(quant_table[i] * scale_coef, 1, 255);
  }

  return quant_table;
}

template<size_t D>
inline QuantizedBlock<D> quantizeBlock(const TransformedBlock<D> &input, const QuantTable<D> &quant_table) {
  QuantizedBlock<D> output;

  for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
    output[pixel_index] = input[pixel_index] / quant_table[pixel_index];
  }

  return output;
}

template<size_t D>
inline TransformedBlock<D> dequantizeBlock(const QuantizedBlock<D> &input, const QuantTable<D> &quant_table) {
  TransformedBlock<D> output {};

  for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
    output[pixel_index] = input[pixel_index] * quant_table[pixel_index];
  }

  return output;
}

template<size_t D>
QuantTable<D> readQuantTable(ifstream &input) {
  QuantTable<D> table {};
  input.read(reinterpret_cast<char *>(table.data()), table.size());
  return table;
}

template<size_t D>
void writeQuantTable(const QuantTable<D> &table, ofstream &output) {
  output.write(reinterpret_cast<const char *>(table.data()), table.size());
}

#endif
