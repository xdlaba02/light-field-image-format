/******************************************************************************\
* SOUBOR: quant_table.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef QUANT_TABLE_H
#define QUANT_TABLE_H

#include "lfiftypes.h"

#include <iosfwd>

using QTABLEUNIT = uint64_t;

template <size_t D>
class QuantTable {
public:
  QuantTable<D> &scaleByQuality(uint8_t quality);
  QuantTable<D> &baseLuma();
  QuantTable<D> &baseChroma();

  QuantTable<D> &writeToStream(std::ofstream &stream);
  QuantTable<D> &readFromStream(std::ifstream &stream);

  QTABLEUNIT operator [](size_t index) const;

private:
  Block<QTABLEUNIT, D> m_block;
};

#endif
