/******************************************************************************\
* SOUBOR: traversal_table.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef TRAVERSAL_TABLE_H
#define TRAVERSAL_TABLE_H

#include "lfiftypes.h"
#include "quant_table.h"

#include <iosfwd>

using REFBLOCKUNIT = double;
using TTABLEUNIT = size_t;

template<size_t D>
using ReferenceBlock = Block<REFBLOCKUNIT, D>;

template <size_t D>
class TraversalTable {
public:
  // TODO Reference as median
  TraversalTable<D> &constructByReference(const ReferenceBlock<D> &reference_block);
  TraversalTable<D> &constructByQuantTable(const QuantTable<D> &quant_table);
  TraversalTable<D> &constructByRadius();
  TraversalTable<D> &constructByDiagonals();
  TraversalTable<D> &constructByHyperboloid();
  TraversalTable<D> &constructZigzag();

  TraversalTable<D> &writeToStream(std::ofstream &stream);
  TraversalTable<D> &readFromStream(std::ifstream &stream);

  TTABLEUNIT operator [](size_t index) const;

private:
  Block<TTABLEUNIT, D> m_block;
};

#endif
