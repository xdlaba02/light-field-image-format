#include "block_decompress_chain.h"

#include "block.h"

#include <algorithm>

template class BlockDecompressChain<2, uint8_t>;
template class BlockDecompressChain<3, uint8_t>;
template class BlockDecompressChain<4, uint8_t>;

template class BlockDecompressChain<2, uint16_t>;
template class BlockDecompressChain<3, uint16_t>;
template class BlockDecompressChain<4, uint16_t>;

template <size_t D, typename T>
BlockDecompressChain<D, T> &
BlockDecompressChain<D, T>::decodeFromStream(HuffmanDecoder huffman_decoders[2], IBitstream &bitstream, T max_rgb_value) {
  RunLengthPair              pair      {};
  std::vector<RunLengthPair> runlength {};

  size_t amp_bits = DCTOutputBits<D>(ceil(log2(max_rgb_value)));

  pair.huffmanDecodeFromStream(huffman_decoders[0], bitstream, amp_bits);
  runlength.push_back(pair);

  do {
    pair.huffmanDecodeFromStream(huffman_decoders[1], bitstream, amp_bits);
    runlength.push_back(pair);
  } while(!pair.endOfBlock());

  m_runlength = runlength;

  return *this;
}

template <size_t D, typename T>
BlockDecompressChain<D, T> &
BlockDecompressChain<D, T>::runLengthDecode() {
  m_traversed_block.fill(0);

  size_t pixel_index = 0;
  for (auto &pair: m_runlength) {
    pixel_index += pair.zeroes;
    m_traversed_block[pixel_index] = pair.amplitude;
    pixel_index++;
  }

  return *this;
}

template <size_t D, typename T>
BlockDecompressChain<D, T> &
BlockDecompressChain<D, T>::detraverse(TraversalTable<D> &traversal_table) {
  for (size_t i = 0; i < constpow(8, D); i++) {
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
  for (size_t i = 0; i < constpow(8, D); i++) {
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
  for (size_t i = 0; i < constpow(8, D); i++) {
    m_ycbcr_block[i] += (max_rgb_value + 1) / 2;
  }

  return *this;
}

template <size_t D, typename T>
BlockDecompressChain<D, T> &
BlockDecompressChain<D, T>::colorConvert(void (*f)(RGBPixel<double> &, YCBCRUNIT, uint16_t), T max_rgb_value) {
  for (size_t i = 0; i < constpow(8, D); i++) {
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

  putBlock<D, T>(inputR, block, img_dims, outputR);
  putBlock<D, T>(inputG, block, img_dims, outputG);
  putBlock<D, T>(inputB, block, img_dims, outputB);

  return *this;
}
