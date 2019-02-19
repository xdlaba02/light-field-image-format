/******************************************************************************\
* SOUBOR: traversal_table.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef TRAVERSAL_TABLE_H
#define TRAVERSAL_TABLE_H

#include "lfiftypes.h"

#include <iosfwd>

using ReferenceBlockUnit = double;
using TraversalTableUnit = int16_t;

template<size_t D>
using ReferenceBlock = Block<ReferenceBlockUnit, D>;

template <size_t D>
class TraversalTable {
public:
  TraversalTable<D> &constructByReference(const ReferenceBlock<D> &reference_block);
  TraversalTable<D> &constructByRadius();
  TraversalTable<D> &constructByDiagonals();

  TraversalTable<D> &writeToStream(std::ofstream &stream);
  TraversalTable<D> &readFromStream(std::ifstream &stream);

  TraversalTableUnit operator [](size_t index) const;

private:
  Block<TraversalTableUnit, D> m_block;
};

#endif
