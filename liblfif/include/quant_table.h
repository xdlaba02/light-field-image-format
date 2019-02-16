/******************************************************************************\
* SOUBOR: quant_table.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef QUANT_TABLE_H
#define QUANT_TABLE_H

#include <algorithm>
#include <fstream>

template <size_t D>
class QuantTable {
public:
  QuantTable<D> &scaleByQuality(uint8_t quality) {
    double scale_coef = quality < 50 ? (5000.0 / quality) / 100 : (200.0 - 2 * quality) / 100;
    for (size_t i = 0; i < constpow(8, D); i++) {
      m_block[i] = clamp<double>(m_block[i] * scale_coef, 1, 255);
    }

    return *this;
  }

  QuantTable<D> &baseLuma() {
    constexpr QuantTableUnit base_luma[64] {
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

  QuantTable<D> &baseChroma() {
    constexpr QuantTableUnit base_chroma[64] {
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

  QuantTable<D> &writeToStream(ofstream &stream) {
    stream.write(reinterpret_cast<const char *>(m_block.data()), m_block.size());

    return *this;
  }

  QuantTable<D> &readFromStream(ifstream &stream) {
    stream.read(reinterpret_cast<char *>(m_block.data()), m_block.size());

    return *this;
  }

  uint8_t operator [](size_t index) const {
    return m_block[index];
  }

private:
  Block<uint8_t, D> m_block;
};

#endif
