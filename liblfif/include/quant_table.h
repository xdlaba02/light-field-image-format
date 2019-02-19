/******************************************************************************\
* SOUBOR: quant_table.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef QUANT_TABLE_H
#define QUANT_TABLE_H

#include "lfiftypes.h"

#include <iosfwd>

template <size_t D, typename T>
class QuantTable {
public:
  QuantTable<D, T> &scaleByQuality(uint8_t quality);
  QuantTable<D, T> &baseLuma();
  QuantTable<D, T> &baseChroma();

  QuantTable<D, T> &writeToStream(std::ofstream &stream);
  QuantTable<D, T> &readFromStream(std::ifstream &stream);

  T operator [](size_t index) const;

private:
  Block<T, D> m_block;
};

#endif
