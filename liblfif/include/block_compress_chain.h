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

template <size_t D, typename T>
class BlockCompressChain {
public:
  BlockCompressChain<D, T> &newRGBBlock(const T *rgb_data, const uint64_t img_dims[D], size_t block);
  BlockCompressChain<D, T> &colorConvert(YCBCRUNIT (*f)(double, double, double, uint16_t), T max_rgb_value);
  BlockCompressChain<D, T> &centerValues(T max_rgb_value);
  BlockCompressChain<D, T> &forwardDiscreteCosineTransform();
  BlockCompressChain<D, T> &quantize(const QuantTable<D> &quant_table);
  BlockCompressChain<D, T> &addToReferenceBlock(ReferenceBlock<D> &reference);
  BlockCompressChain<D, T> &diffEncodeDC(QDATAUNIT &previous_DC);
  BlockCompressChain<D, T> &traverse(const TraversalTable<D> &traversal_table);
  BlockCompressChain<D, T> &runLengthEncode(size_t max_zeroes);
  BlockCompressChain<D, T> &huffmanAddWeights(HuffmanWeights weights[2], size_t class_bits);
  BlockCompressChain<D, T> &encodeToStream(HuffmanEncoder encoder[2], OBitstream &stream, size_t class_bits);

private:
  Block<RGBPixel<T>, D>      m_rgb_block;
  Block<YCBCRUNIT,   D>      m_ycbcr_block;
  Block<DCTDATAUNIT, D>      m_transformed_block;
  Block<QDATAUNIT,   D>      m_quantized_block;
  Block<QDATAUNIT,   D>      m_traversed_block;
  Block<RunLengthPair, D>    m_runlength;
};

template <size_t D, typename T>
BlockCompressChain<D, T> &
BlockCompressChain<D, T>::newRGBBlock(const T *rgb_data, const uint64_t img_dims[D], size_t block) {
  auto inputR = [&](size_t index) {
    return rgb_data[index * 3 + 0];
  };

  auto inputG = [&](size_t index) {
    return rgb_data[index * 3 + 1];
  };

  auto inputB = [&](size_t index) {
    return rgb_data[index * 3 + 2];
  };


  auto outputR = [&](size_t index) -> T & {
    return m_rgb_block[index].r;
  };

  auto outputG = [&](size_t index) -> T & {
    return m_rgb_block[index].g;
  };

  auto outputB = [&](size_t index) -> T & {
    return m_rgb_block[index].b;
  };

  getBlock<D>(inputR, block, img_dims, outputR);
  getBlock<D>(inputG, block, img_dims, outputG);
  getBlock<D>(inputB, block, img_dims, outputB);

  return *this;
}

template <size_t D, typename T>
BlockCompressChain<D, T> &
BlockCompressChain<D, T>::colorConvert(YCBCRUNIT (*f)(double, double, double, uint16_t), T max_rgb_value) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    double R = m_rgb_block[i].r;
    double G = m_rgb_block[i].g;
    double B = m_rgb_block[i].b;

    m_ycbcr_block[i] = f(R, G, B, max_rgb_value);
  }

  return *this;
}

template <size_t D, typename T>
BlockCompressChain<D, T> &
BlockCompressChain<D, T>::centerValues(T max_rgb_value) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    m_ycbcr_block[i] -= (max_rgb_value + 1) / 2;
  }

  return *this;
}

template <size_t D, typename T>
BlockCompressChain<D, T> &
BlockCompressChain<D, T>::forwardDiscreteCosineTransform() {
  m_transformed_block.fill(0);

  fdct<D>([&](size_t index) -> DCTDATAUNIT { return m_ycbcr_block[index];}, [&](size_t index) -> DCTDATAUNIT & { return m_transformed_block[index]; });

  return *this;
}

template <size_t D, typename T>
BlockCompressChain<D, T> &
BlockCompressChain<D, T>::quantize(const QuantTable<D> &quant_table) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    m_quantized_block[i] = m_transformed_block[i] / quant_table[i];
  }

  return *this;
}

template <size_t D, typename T>
BlockCompressChain<D, T> &
BlockCompressChain<D, T>::addToReferenceBlock(ReferenceBlock<D> &reference) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    reference[i] += abs(m_quantized_block[i]);
  }

  return *this;
}

template <size_t D, typename T>
BlockCompressChain<D, T> &
BlockCompressChain<D, T>::diffEncodeDC(QDATAUNIT &previous_DC) {
  QDATAUNIT current_DC = m_quantized_block[0];
  m_quantized_block[0] -= previous_DC;
  previous_DC = current_DC;

  return *this;
}

template <size_t D, typename T>
BlockCompressChain<D, T> &
BlockCompressChain<D, T>::traverse(const TraversalTable<D> &traversal_table) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    m_traversed_block[traversal_table[i]] = m_quantized_block[i];
  }

  return *this;
}

template <size_t D, typename T>
BlockCompressChain<D, T> &
BlockCompressChain<D, T>::runLengthEncode(size_t max_zeroes) {
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

template <size_t D, typename T>
BlockCompressChain<D, T> &
BlockCompressChain<D, T>::huffmanAddWeights(HuffmanWeights weights[2], size_t class_bits) {
  auto pairs_it = std::begin(m_runlength);

  pairs_it->addToWeights(weights[0], class_bits);

  do {
    pairs_it++;
    pairs_it->addToWeights(weights[1], class_bits);
  } while (!pairs_it->eob() && (pairs_it != (std::end(m_runlength) - 1)));

  return *this;
}

template <size_t D, typename T>
BlockCompressChain<D, T> &
BlockCompressChain<D, T>::encodeToStream(HuffmanEncoder encoder[2], OBitstream &stream, size_t class_bits) {
  auto pairs_it = std::begin(m_runlength);

  pairs_it->huffmanEncodeToStream(encoder[0], stream, class_bits);

  do {
    pairs_it++;
    pairs_it->huffmanEncodeToStream(encoder[1], stream, class_bits);
  } while (!pairs_it->eob() && (pairs_it != (std::end(m_runlength) - 1)));

  return *this;
}


#endif
