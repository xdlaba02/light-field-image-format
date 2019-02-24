#include "quant_table.h"

#include <algorithm>

template class QuantTable<2>;
template class QuantTable<3>;
template class QuantTable<4>;

using namespace std;

template <size_t D>
QuantTable<D> &
QuantTable<D>::scaleByQuality(uint8_t quality) {
  double scale_coef = quality < 50 ? (5000.0 / quality) / 100 : (200.0 - 2 * quality) / 100;

  for (size_t i = 0; i < constpow(8, D); i++) {
    m_block[i] = clamp<double>(m_block[i] * scale_coef, 1, 255);
  }

  return *this;
}

template <size_t D>
QuantTable<D> &
QuantTable<D>::baseLuma() {
  constexpr QTABLEUNIT base_luma[64] {
    16, 11, 10, 16, 124, 140, 151, 161,
    12, 12, 14, 19, 126, 158, 160, 155,
    14, 13, 16, 24, 140, 157, 169, 156,
    14, 17, 22, 29, 151, 187, 180, 162,
    18, 22, 37, 56, 168, 109, 103, 177,
    24, 35, 55, 64, 181, 104, 113, 192,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 199,
  };

  for (size_t i = 0; i < constpow(8, D); i++) {
    m_block[i] = base_luma[i%64];
  }

  return *this;
}

template <size_t D>
QuantTable<D> &
QuantTable<D>::baseChroma() {
  constexpr QTABLEUNIT base_chroma[64] {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
  };

  for (size_t i = 0; i < constpow(8, D); i++) {
    m_block[i] = base_chroma[i%64];
  }

  return *this;
}

template <size_t D>
QuantTable<D> &
QuantTable<D>::writeToStream(std::ofstream &stream) {
  for (size_t i = 0; i < constpow(8, D); i++) {
    writeValueToStream<uint8_t>(stream, m_block[i]);
  }

  return *this;
}

template <size_t D>
QuantTable<D> &
QuantTable<D>::readFromStream(std::ifstream &stream) {
  for (size_t i = 0; i < constpow(8, D); i++) {
    m_block[i] = readValueFromStream<uint8_t>(stream);
  }
  return *this;
}

template <size_t D>
uint64_t QuantTable<D>::operator [](size_t index) const {
  return m_block[index];
}
