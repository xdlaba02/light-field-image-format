/******************************************************************************\
* SOUBOR: traversal_table.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef TRAVERSAL_TABLE_H
#define TRAVERSAL_TABLE_H

using TraversalTableUnit = uint16_t;
using ReferenceBlockUnit = double;

template<size_t D>
using ReferenceBlock = Block<ReferenceBlockUnit, D>;

template <size_t D>
class TraversalTable {
public:
  TraversalTable<D> &constructByReference(const ReferenceBlock<D> &reference_block) {
    Block<std::pair<double, size_t>, D> srt {};

    for (size_t i = 0; i < constpow(8, D); i++) {
      srt[i].first += abs(reference_block[i]);
      srt[i].second = i;
    }

    stable_sort(srt.rbegin(), srt.rend());

    for (size_t i = 0; i < constpow(8, D); i++) {
      m_block[srt[i].second] = i;
    }

    return *this;
  }

  TraversalTable<D> &constructByRadius() {
    Block<std::pair<double, size_t>, D> srt {};

    for (size_t i = 0; i < constpow(8, D); i++) {
      for (size_t j = 1; j <= D; j++) {
        size_t coord = (i % constpow(8, j)) / constpow(8, j-1);
        srt[i].first += coord * coord;
      }
      srt[i].second = i;
    }

    stable_sort(srt.begin(), srt.end());

    for (size_t i = 0; i < constpow(8, D); i++) {
      m_block[srt[i].second] = i;
    }

    return *this;
  }

  TraversalTable<D> &constructByDiagonals() {
    Block<std::pair<double, size_t>, D> srt {};

    for (size_t i = 0; i < constpow(8, D); i++) {
      for (size_t j = 1; j <= D; j++) {
        srt[i].first += (i % constpow(8, j)) / constpow(8, j-1);
      }
      srt[i].second = i;
    }

    stable_sort(srt.begin(), srt.end());

    for (size_t i = 0; i < constpow(8, D); i++) {
      m_block[srt[i].second] = i;
    }

    return *this;
  }

  TraversalTable<D> &writeToStream(std::ofstream &stream) {
    for (size_t i = 0; i < m_block.size(); i++) {
      TraversalTableUnit val = htobe16(m_block[i]);
      stream.write(reinterpret_cast<const char *>(&val), sizeof(val));
    }

    return *this;
  }

  TraversalTable<D> &readFromStream(std::ifstream &stream) {
    stream.read(reinterpret_cast<char *>(m_block.data()), m_block.size() * sizeof(TraversalTableUnit));

    for (size_t i = 0; i < m_block.size(); i++) {
      m_block[i] = be16toh(m_block[i]);
    }

    return *this;
  }

  uint16_t operator [](size_t index) const {
    return m_block[index];
  }

private:
  Block<TraversalTableUnit, D> m_block;
};

#endif
