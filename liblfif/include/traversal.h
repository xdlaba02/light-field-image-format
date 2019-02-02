/******************************************************************************\
* SOUBOR: traversal.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef TRAVERSAL_H
#define TRAVERSAL_H

template<size_t D>
inline TraversalTable<D> constructTraversalTableByReference(const RefereceBlock<D> &ref_block) {
  TraversalTable<D> traversal_table    {};
  Block<pair<double, size_t>, D> srt {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    srt[i].first += abs(ref_block[i]);
    srt[i].second = i;
  }

  stable_sort(srt.rbegin(), srt.rend());

  for (size_t i = 0; i < constpow(8, D); i++) {
    traversal_table[srt[i].second] = i;
  }

  return traversal_table;
}

template<size_t D>
TraversalTable<D> constructTraversalTableByRadius() {
  TraversalTable<D>                  traversal_table {};
  Block<pair<double, size_t>, D> srt          {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    for (size_t j = 1; j <= D; j++) {
      size_t coord = (i % constpow(8, j)) / constpow(8, j-1);
      srt[i].first += coord * coord;
    }
    srt[i].second = i;
  }

  stable_sort(srt.begin(), srt.end());

  for (size_t i = 0; i < constpow(8, D); i++) {
    traversal_table[srt[i].second] = i;
  }

  return traversal_table;
}

template<size_t D>
inline TraversalTable<D> constructTraversalTableByDiagonals() {
  TraversalTable<D>              traversal_table {};
  Block<pair<double, size_t>, D> srt          {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    for (size_t j = 1; j <= D; j++) {
      srt[i].first += (i % constpow(8, j)) / constpow(8, j-1);
    }
    srt[i].second = i;
  }

  stable_sort(srt.begin(), srt.end());

  for (size_t i = 0; i < constpow(8, D); i++) {
    traversal_table[srt[i].second] = i;
  }

  return traversal_table;
}

#endif
