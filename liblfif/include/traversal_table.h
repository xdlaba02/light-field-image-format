/**
* @file traversal_table.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Module for generating linearization matrices.
*/

#ifndef TRAVERSAL_TABLE_H
#define TRAVERSAL_TABLE_H

#include "quant_table.h"
#include "zigzag.h"
#include "endian_t.h"

#include <iosfwd>

using REFBLOCKUNIT = double; /**< @brief Type intended to be used in reference block.*/
using TTABLEUNIT = size_t;   /**< @brief Type intended to be used in traversal matrix.*/

/**
 * @brief Reference block type.
 */
template<size_t BS, size_t D>
using ReferenceBlock = Block<REFBLOCKUNIT, BS, D>;

/**
 * @brief Traversal table class.
 */
template <size_t BS, size_t D>
class TraversalTable {
public:

  /**
   * @brief Method which constructs traversal matrix by reference block.
   * @param reference_block The block from which the matrix shall be constructed.
   * @return Reference to itself.
   */
  TraversalTable<BS, D> &constructByReference(const ReferenceBlock<BS, D> &reference_block);

  /**
   * @brief Method which constructs traversal matrix from quantization matrix.
   * @param quant_table The quantization matrix from which the matrix shall be constructed.
   * @return Reference to itself.
   */
  TraversalTable<BS, D> &constructByQuantTable(const QuantTable<BS, D> &quant_table);

  /**
   * @brief Method which constructs traversal matrix by Eucleidian distance from the DC coefficient.
   * @return Reference to itself.
   */
  TraversalTable<BS, D> &constructByRadius();

  /**
   * @brief Method which constructs traversal matrix by manhattan distance from the DC coefficient.
   * @return Reference to itself.
   */
  TraversalTable<BS, D> &constructByDiagonals();

  /**
   * @brief Method which constructs traversal matrix by Eucleidian distance from the AC coefficient with highest frequency.
   * @return Reference to itself.
   */
  TraversalTable<BS, D> &constructByHyperboloid();

  /**
   * @brief Method which constructs zig-zag matrix.
   * @return Reference to itself.
   */
  TraversalTable<BS, D> &constructZigzag();

  /**
   * @brief Method which writes the matrix to stream.
   * @param stream The stream to which the table shall be written.
   * @return Reference to itself.
   */
  TraversalTable<BS, D> &writeToStream(std::ostream &stream);

  /**
   * @brief Method which read the matrix from stream.
   * @param stream The stream from which the table shall be read.
   * @return Reference to itself.
   */
  TraversalTable<BS, D> &readFromStream(std::istream &stream);

  /**
   * @brief Method used to index the internal matrix.
   * @param index The index of a linearization index.
   * @return Linearization index.
   */
  TTABLEUNIT operator [](size_t index) const {
    return m_block[index];
  }

private:
  Block<TTABLEUNIT, BS, D> m_block;
};

template <size_t BS, size_t D>
TraversalTable<BS, D> &TraversalTable<BS, D>::constructByReference(const ReferenceBlock<BS, D> &reference_block) {
  Block<std::pair<double, size_t>, BS, D> srt {};

  for (size_t i = 0; i < constpow(BS, D); i++) {
    srt[i].first += reference_block[i];
    srt[i].second = i;
  }

  //do NOT sort DC coefficient, thus +1 at the begining.
  stable_sort(srt.begin() + 1, srt.end(), [](auto &left, auto &right) {
    return left.first > right.first;
  });

  for (size_t i = 0; i < constpow(BS, D); i++) {
    m_block[srt[i].second] = i;
  }

  return *this;
}

template <size_t BS, size_t D>
TraversalTable<BS, D> &
TraversalTable<BS, D>::constructByQuantTable(const QuantTable<BS, D> &quant_table) {
  ReferenceBlock<BS, D> dummy {};

  for (size_t i = 0; i < constpow(BS, D); i++) {
    dummy[i] = quant_table[i];
    dummy[i] *= -1;
  }

  return constructByReference(dummy);
}

template <size_t BS, size_t D>
TraversalTable<BS, D> &
TraversalTable<BS, D>::constructByRadius() {
  ReferenceBlock<BS, D> dummy {};

  for (size_t i = 0; i < constpow(BS, D); i++) {
    for (size_t j = 1; j <= D; j++) {
      size_t coord = (i % constpow(BS, j)) / constpow(BS, j-1);
      dummy[i] += coord * coord;
    }
    dummy[i] *= -1;
  }

  return constructByReference(dummy);
}

template <size_t BS, size_t D>
TraversalTable<BS, D> &
TraversalTable<BS, D>::constructByDiagonals() {
  ReferenceBlock<BS, D> dummy {};

  for (size_t i = 0; i < constpow(BS, D); i++) {
    for (size_t j = 1; j <= D; j++) {
      dummy[i] += (i % constpow(BS, j)) / constpow(BS, j-1);
    }
    dummy[i] *= -1;
  }

  return constructByReference(dummy);
}

template <size_t BS, size_t D>
TraversalTable<BS, D> &
TraversalTable<BS, D>::constructByHyperboloid() {
  ReferenceBlock<BS, D> dummy {};

  dummy.fill(1);

  for (size_t i = 0; i < constpow(BS, D); i++) {
    for (size_t j = 1; j <= D; j++) {
      size_t coord = (i % constpow(BS, j)) / constpow(BS, j-1);
      dummy[i] *= (coord + 1);
    }
    dummy[i] *= -1;
  }

  return constructByReference(dummy);
}

template <size_t BS, size_t D>
TraversalTable<BS, D> &
TraversalTable<BS, D>::constructZigzag() {
  m_block = generateZigzagTable<BS, D>();

  return *this;
}

template <size_t BS, size_t D>
TraversalTable<BS, D> &
TraversalTable<BS, D>::writeToStream(std::ostream &stream) {
  size_t max_bits = ceil(log2(constpow(BS, D)));

  if (max_bits <= 8) {
    for (size_t i = 0; i < constpow(BS, D); i++) {
      writeValueToStream<uint8_t>(m_block[i], stream);
    }
  }
  else if (max_bits <= 16) {
    for (size_t i = 0; i < constpow(BS, D); i++) {
      writeValueToStream<uint16_t>(m_block[i], stream);
    }
  }
  else if (max_bits <= 32) {
    for (size_t i = 0; i < constpow(BS, D); i++) {
      writeValueToStream<uint32_t>(m_block[i], stream);
    }
  }
  else if (max_bits <= 64) {
    for (size_t i = 0; i < constpow(BS, D); i++) {
      writeValueToStream<uint64_t>(m_block[i], stream);
    }
  }

  return *this;
}


template <size_t BS, size_t D>
TraversalTable<BS, D> &
TraversalTable<BS, D>::readFromStream(std::istream &stream) {
  size_t max_bits = ceil(log2(constpow(BS, D)));

  if (max_bits <= 8) {
    for (size_t i = 0; i < constpow(BS, D); i++) {
      m_block[i] = readValueFromStream<uint8_t>(stream);
    }
  }
  else if (max_bits <= 16) {
    for (size_t i = 0; i < constpow(BS, D); i++) {
      m_block[i] = readValueFromStream<uint16_t>(stream);
    }
  }
  else if (max_bits <= 32) {
    for (size_t i = 0; i < constpow(BS, D); i++) {
      m_block[i] = readValueFromStream<uint32_t>(stream);
    }
  }
  else if (max_bits <= 64) {
    for (size_t i = 0; i < constpow(BS, D); i++) {
      m_block[i] = readValueFromStream<uint64_t>(stream);
    }
  }

  return *this;
}

#endif
