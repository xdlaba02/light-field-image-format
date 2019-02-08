/******************************************************************************\
* SOUBOR: traversal.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef TRAVERSAL_H
#define TRAVERSAL_H

template<size_t D>
void addToReference(const QuantizedBlock<D>& block, RefereceBlock<D>& ref) {
  for (size_t i = 0; i < constpow(8, D); i++) {
    ref[i] += abs(block[i]);
  }
}

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

template<size_t D>
TraversalTable<D> readTraversalTable(ifstream &input) {
  TraversalTable<D> table {};

  input.read(reinterpret_cast<char *>(table.data()), table.size() * sizeof(TraversalTableUnit));

  for (size_t i = 0; i < table.size(); i++) {
    table[i] = be16toh(table[i]);
  }

  return table;
}

template<size_t D>
void writeTraversalTable(const TraversalTable<D> &table, ofstream &output) {
  for (size_t i = 0; i < table.size(); i++) {
    TraversalTableUnit val = htobe16(table[i]);
    output.write(reinterpret_cast<const char *>(&val), sizeof(val));
  }
}

template<size_t D>
inline TraversedBlock<D> traverseBlock(const QuantizedBlock<D> &input, const TraversalTable<D> &traversal_table) {
  TraversedBlock<D> output {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    output[traversal_table[i]] = input[i];
  }

  return output;
}

template<size_t D>
inline QuantizedBlock<D> detraverseBlock(const TraversedBlock<D> &input, const TraversalTable<D> &traversal_table) {
  QuantizedBlock<D> output {};

  for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
    output[pixel_index] = input[traversal_table[pixel_index]];
  }

  return output;
}

#endif
