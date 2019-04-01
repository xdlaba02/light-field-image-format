/******************************************************************************\
* SOUBOR: block_compress_chain.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef BLOCK_COMPRESS_CHAIN
#define BLOCK_COMPRESS_CHAIN

#include "lfiftypes.h"
#include "quant_table.h"
#include "traversal_table.h"
#include "huffman.h"
#include "dct.h"
#include "block.h"
#include "runlength.h"

#include <cstdlib>
#include <cstdint>

template <size_t D>
class BlockCompressChain {
public:
  BlockCompressChain<D> &forwardDiscreteCosineTransform();
  BlockCompressChain<D> &quantize(const QuantTable<D> &quant_table);
  BlockCompressChain<D> &addToReferenceBlock(ReferenceBlock<D> &reference);
  BlockCompressChain<D> &diffEncodeDC(QDATAUNIT &previous_DC);
  BlockCompressChain<D> &traverse(const TraversalTable<D> &traversal_table);
  BlockCompressChain<D> &runLengthEncode(size_t max_zeroes);
  BlockCompressChain<D> &huffmanAddWeights(HuffmanWeights weights[2], size_t class_bits);
  BlockCompressChain<D> &encodeToStream(HuffmanEncoder encoder[2], OBitstream &stream, size_t class_bits);

  Block<YCBCRUNIT,     D>    m_ycbcr_block;
  Block<DCTDATAUNIT,   D>    m_transformed_block;
  Block<QDATAUNIT,     D>    m_quantized_block;
  Block<QDATAUNIT,     D>    m_traversed_block;
  Block<RunLengthPair, D>    m_runlength;
};

template <size_t D>
BlockCompressChain<D> &
BlockCompressChain<D>::forwardDiscreteCosineTransform() {
  m_transformed_block.fill(0);

  fdct<D>([&](size_t index) -> DCTDATAUNIT { return m_ycbcr_block[index];}, [&](size_t index) -> DCTDATAUNIT & { return m_transformed_block[index]; });

  return *this;
}

template <size_t D>
BlockCompressChain<D> &
BlockCompressChain<D>::quantize(const QuantTable<D> &quant_table) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    m_quantized_block[i] = m_transformed_block[i] / quant_table[i];
  }

  return *this;
}

template <size_t D>
BlockCompressChain<D> &
BlockCompressChain<D>::addToReferenceBlock(ReferenceBlock<D> &reference) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    reference[i] += abs(m_quantized_block[i]);
  }

  return *this;
}

template <size_t D>
BlockCompressChain<D> &
BlockCompressChain<D>::diffEncodeDC(QDATAUNIT &previous_DC) {
  QDATAUNIT current_DC = m_quantized_block[0];
  m_quantized_block[0] -= previous_DC;
  previous_DC = current_DC;

  return *this;
}

template <size_t D>
BlockCompressChain<D> &
BlockCompressChain<D>::traverse(const TraversalTable<D> &traversal_table) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    m_traversed_block[traversal_table[i]] = m_quantized_block[i];
  }

  return *this;
}

template <size_t D>
BlockCompressChain<D> &
BlockCompressChain<D>::runLengthEncode(size_t max_zeroes) {
  auto pairs_it = std::begin(m_runlength);

  auto push_pair = [&pairs_it](RunLengthPair &&pair) {
    *pairs_it = pair;
    pairs_it++;
  };

  push_pair({0, m_traversed_block[0]});

  size_t zeroes = 0;
  for (size_t i = 1; i < constpow(BLOCK_SIZE, D); i++) {
    if (m_traversed_block[i] == 0) {
      zeroes++;
    }
    else {
      while (zeroes >= max_zeroes) {
        push_pair({max_zeroes - 1, 0});
        zeroes -= max_zeroes;
      }
      push_pair({zeroes, m_traversed_block[i]});
      zeroes = 0;
    }
  }

  push_pair({0, 0});

  return *this;
}

template <size_t D>
BlockCompressChain<D> &
BlockCompressChain<D>::huffmanAddWeights(HuffmanWeights weights[2], size_t class_bits) {
  auto pairs_it = std::begin(m_runlength);

  pairs_it->addToWeights(weights[0], class_bits);

  do {
    pairs_it++;
    pairs_it->addToWeights(weights[1], class_bits);
  } while (!pairs_it->eob() && (pairs_it != (std::end(m_runlength) - 1)));

  return *this;
}

template <size_t D>
BlockCompressChain<D> &
BlockCompressChain<D>::encodeToStream(HuffmanEncoder encoder[2], OBitstream &stream, size_t class_bits) {
  auto pairs_it = std::begin(m_runlength);

  pairs_it->huffmanEncodeToStream(encoder[0], stream, class_bits);

  do {
    pairs_it++;
    pairs_it->huffmanEncodeToStream(encoder[1], stream, class_bits);
  } while (!pairs_it->eob() && (pairs_it != (std::end(m_runlength) - 1)));

  return *this;
}


#endif
