#include "traversal_table.h"
#include "zigzag.h"

#include <cmath>
#include <algorithm>

template class TraversalTable<2>;
template class TraversalTable<3>;
template class TraversalTable<4>;

template <size_t D>
TraversalTable<D> &
TraversalTable<D>::constructByReference(const ReferenceBlock<D> &reference_block) {
  Block<std::pair<double, size_t>, D> srt {};

  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    srt[i].first += reference_block[i];
    srt[i].second = i;
  }

  //do NOT sort DC coefficient, thus +1 at the begining.
  stable_sort(srt.begin() + 1, srt.end(), [](auto &left, auto &right) {
    return left.first > right.first;
  });

  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    m_block[srt[i].second] = i;
  }

  return *this;
}

template <size_t D>
TraversalTable<D> &
TraversalTable<D>::constructByQuantTable(const QuantTable<D> &quant_table) {
  ReferenceBlock<D> dummy {};

  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    dummy[i] = quant_table[i];
    dummy[i] *= -1;
  }

  return constructByReference(dummy);
}

template <size_t D>
TraversalTable<D> &
TraversalTable<D>::constructByRadius() {
  ReferenceBlock<D> dummy {};

  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    for (size_t j = 1; j <= D; j++) {
      size_t coord = (i % constpow(BLOCK_SIZE, j)) / constpow(BLOCK_SIZE, j-1);
      dummy[i] += coord * coord;
    }
    dummy[i] *= -1;
  }

  return constructByReference(dummy);
}

template <size_t D>
TraversalTable<D> &
TraversalTable<D>::constructByDiagonals() {
  ReferenceBlock<D> dummy {};

  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    for (size_t j = 1; j <= D; j++) {
      dummy[i] += (i % constpow(BLOCK_SIZE, j)) / constpow(BLOCK_SIZE, j-1);
    }
    dummy[i] *= -1;
  }

  return constructByReference(dummy);
}

template <size_t D>
TraversalTable<D> &
TraversalTable<D>::constructByHyperboloid() {
  ReferenceBlock<D> dummy {};

  dummy.fill(1);

  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    for (size_t j = 1; j <= D; j++) {
      size_t coord = (i % constpow(BLOCK_SIZE, j)) / constpow(BLOCK_SIZE, j-1);
      dummy[i] *= (coord + 1);
    }
    dummy[i] *= -1;
  }

  return constructByReference(dummy);
}

template <size_t D>
TraversalTable<D> &
TraversalTable<D>::constructZigzag() {
  m_block = generateZigzagTable<D>();

  return *this;
}

template <size_t D>
TraversalTable<D> &
TraversalTable<D>::writeToStream(std::ofstream &stream) {
  size_t max_bits = ceil(log2(constpow(BLOCK_SIZE, D)));

  if (max_bits <= 8) {
    for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
      writeValueToStream<uint8_t>(stream, m_block[i]);
    }
  }
  else if (max_bits <= 16) {
    for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
      writeValueToStream<uint16_t>(stream, m_block[i]);
    }
  }
  else if (max_bits <= 32) {
    for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
      writeValueToStream<uint32_t>(stream, m_block[i]);
    }
  }
  else if (max_bits <= 64) {
    for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
      writeValueToStream<uint64_t>(stream, m_block[i]);
    }
  }

  return *this;
}


template <size_t D>
TraversalTable<D> &
TraversalTable<D>::readFromStream(std::ifstream &stream) {
  size_t max_bits = ceil(log2(constpow(BLOCK_SIZE, D)));

  if (max_bits <= 8) {
    for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
      m_block[i] = readValueFromStream<uint8_t>(stream);
    }
  }
  else if (max_bits <= 16) {
    for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
      m_block[i] = readValueFromStream<uint16_t>(stream);
    }
  }
  else if (max_bits <= 32) {
    for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
      m_block[i] = readValueFromStream<uint32_t>(stream);
    }
  }
  else if (max_bits <= 64) {
    for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
      m_block[i] = readValueFromStream<uint64_t>(stream);
    }
  }

  return *this;
}


template <size_t D>
TTABLEUNIT TraversalTable<D>::operator [](size_t index) const {
  return m_block[index];
}
