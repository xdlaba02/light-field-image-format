/******************************************************************************\
* SOUBOR: block_decompress_chain.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef BLOCK_DECOMPRESS_CHAIN
#define BLOCK_DECOMPRESS_CHAIN

#include "lfiftypes.h"
#include "huffman.h"
#include "runlength.h"
#include "traversal_table.h"
#include "quant_table.h"
#include "block.h"
#include "dct.h"

#include <vector>

template <size_t D, typename T>
class BlockDecompressChain {
public:
  BlockDecompressChain<D, T> &decodeFromStream(HuffmanDecoder huffman_decoders[2], IBitstream &bitstream, size_t class_bits);
  BlockDecompressChain<D, T> &runLengthDecode();
  BlockDecompressChain<D, T> &detraverse(TraversalTable<D> &traversal_table);
  BlockDecompressChain<D, T> &diffDecodeDC(QDATAUNIT &previous_DC);
  BlockDecompressChain<D, T> &dequantize(QuantTable<D> &quant_table);
  BlockDecompressChain<D, T> &inverseDiscreteCosineTransform();
  BlockDecompressChain<D, T> &decenterValues(T max_rgb_value);
  BlockDecompressChain<D, T> &colorConvert(void (*f)(RGBPixel<double> &, YCBCRUNIT, uint16_t), T max_rgb_value);
  BlockDecompressChain<D, T> &putRGBBlock(T *rgb_data, const uint64_t img_dims[D], size_t block, T max_rgb_value);

private:
  Block<RunLengthPair,    D> m_runlength;
  Block<QDATAUNIT,        D> m_traversed_block;
  Block<QDATAUNIT,        D> m_quantized_block;
  Block<DCTDATAUNIT,      D> m_transformed_block;
  Block<YCBCRUNIT,        D> m_ycbcr_block;
  Block<RGBPixel<double>, D> m_rgb_block;
};

template <size_t D, typename T>
BlockDecompressChain<D, T> &
BlockDecompressChain<D, T>::decodeFromStream(HuffmanDecoder huffman_decoders[2], IBitstream &bitstream, size_t class_bits) {
  auto pairs_it = std::begin(m_runlength);

  pairs_it->huffmanDecodeFromStream(huffman_decoders[0], bitstream, class_bits);

  do {
    pairs_it++;
    pairs_it->huffmanDecodeFromStream(huffman_decoders[1], bitstream, class_bits);
  } while(!pairs_it->eob() && (pairs_it != (std::end(m_runlength) - 1)));

  return *this;
}

template <size_t D, typename T>
BlockDecompressChain<D, T> &
BlockDecompressChain<D, T>::runLengthDecode() {
  m_traversed_block.fill(0);

  size_t pixel_index = 0;
  auto pairs_it = std::begin(m_runlength);
  do {
    pixel_index += pairs_it->zeroes;
    m_traversed_block[pixel_index] = pairs_it->amplitude;
    pixel_index++;
    pairs_it++;
  } while (!pairs_it->eob() && (pairs_it != std::end(m_runlength)));

  return *this;
}

template <size_t D, typename T>
BlockDecompressChain<D, T> &
BlockDecompressChain<D, T>::detraverse(TraversalTable<D> &traversal_table) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    m_quantized_block[i] = m_traversed_block[traversal_table[i]];
  }

  return *this;
}

template <size_t D, typename T>
BlockDecompressChain<D, T> &
BlockDecompressChain<D, T>::diffDecodeDC(QDATAUNIT &previous_DC) {
  m_quantized_block[0] += previous_DC;
  previous_DC = m_quantized_block[0];

  return *this;
}

template <size_t D, typename T>
BlockDecompressChain<D, T> &
BlockDecompressChain<D, T>::dequantize(QuantTable<D> &quant_table) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    m_transformed_block[i] = static_cast<double>(m_quantized_block[i]) * quant_table[i];
  }

  return *this;
}

template <size_t D, typename T>
BlockDecompressChain<D, T> &
BlockDecompressChain<D, T>::inverseDiscreteCosineTransform() {
  m_ycbcr_block.fill(0);

  idct<D>([&](size_t index) -> DCTDATAUNIT { return m_transformed_block[index];}, [&](size_t index) -> DCTDATAUNIT & { return m_ycbcr_block[index]; });

  return *this;
}

template <size_t D, typename T>
BlockDecompressChain<D, T> &
BlockDecompressChain<D, T>::decenterValues(T max_rgb_value) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    m_ycbcr_block[i] += (max_rgb_value + 1) / 2;
  }

  return *this;
}

template <size_t D, typename T>
BlockDecompressChain<D, T> &
BlockDecompressChain<D, T>::colorConvert(void (*f)(RGBPixel<double> &, YCBCRUNIT, uint16_t), T max_rgb_value) {
  for (size_t i = 0; i < constpow(BLOCK_SIZE, D); i++) {
    f(m_rgb_block[i], m_ycbcr_block[i], max_rgb_value);
  }

  return *this;
}

template <size_t D, typename T>
BlockDecompressChain<D, T> &
BlockDecompressChain<D, T>::putRGBBlock(T *rgb_data, const uint64_t img_dims[D], size_t block, T max_rgb_value) {
  auto inputR = [&](size_t index) {
    return std::clamp<double>(m_rgb_block[index].r, 0, max_rgb_value);
  };

  auto inputG = [&](size_t index) {
    return std::clamp<double>(m_rgb_block[index].g, 0, max_rgb_value);
  };

  auto inputB = [&](size_t index) {
    return std::clamp<double>(m_rgb_block[index].b, 0, max_rgb_value);
  };


  auto outputR = [&](size_t index) -> T & {
    return rgb_data[index * 3 + 0];
  };

  auto outputG = [&](size_t index) -> T & {
    return rgb_data[index * 3 + 1];
  };

  auto outputB = [&](size_t index) -> T & {
    return rgb_data[index * 3 + 2];
  };

  putBlock<D>(inputR, block, img_dims, outputR);
  putBlock<D>(inputG, block, img_dims, outputG);
  putBlock<D>(inputB, block, img_dims, outputB);

  return *this;
}


#endif
