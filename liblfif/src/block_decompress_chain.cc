#include "block_decompress_chain.h"

#include "block.h"

#include <algorithm>

template class BlockDecompressChain<2, uint8_t, int16_t>;
template class BlockDecompressChain<3, uint8_t, int16_t>;
template class BlockDecompressChain<4, uint8_t, int16_t>;

template class BlockDecompressChain<2, uint16_t, int32_t>;
template class BlockDecompressChain<3, uint16_t, int32_t>;
template class BlockDecompressChain<4, uint16_t, int32_t>;

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &
BlockDecompressChain<D, RGBUNIT, QDATAUNIT>::decodeFromStream(HuffmanDecoder huffman_decoders[2], IBitstream &bitstream) {
  RunLengthPair<QDATAUNIT>              pair      {};
  std::vector<RunLengthPair<QDATAUNIT>> runlength {};

  pair.huffmanDecodeFromStream(huffman_decoders[0], bitstream);
  runlength.push_back(pair);

  do {
    pair.huffmanDecodeFromStream(huffman_decoders[1], bitstream);
    runlength.push_back(pair);
  } while(!pair.endOfBlock());

  m_runlength = runlength;

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &
BlockDecompressChain<D, RGBUNIT, QDATAUNIT>::runLengthDecode() {
  m_traversed_block.fill(0);

  size_t pixel_index = 0;
  for (auto &pair: m_runlength) {
    pixel_index += pair.zeroes;
    m_traversed_block[pixel_index] = pair.amplitude;
    pixel_index++;
  }

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &
BlockDecompressChain<D, RGBUNIT, QDATAUNIT>::detraverse(TraversalTable<D> &traversal_table) {
  for (size_t i = 0; i < constpow(8, D); i++) {
    m_quantized_block[i] = m_traversed_block[traversal_table[i]];
  }

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &
BlockDecompressChain<D, RGBUNIT, QDATAUNIT>::diffDecodeDC(QDATAUNIT &previous_DC) {
  m_quantized_block[0] += previous_DC;
  previous_DC = m_quantized_block[0];

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &
BlockDecompressChain<D, RGBUNIT, QDATAUNIT>::dequantize(QuantTable<D, RGBUNIT> &quant_table) {
  for (size_t i = 0; i < constpow(8, D); i++) {
    m_transformed_block[i] = m_quantized_block[i] * quant_table[i];
  }

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &
BlockDecompressChain<D, RGBUNIT, QDATAUNIT>::inverseDiscreteCosineTransform() {
  m_ycbcr_block.fill(0);

  idct<D>([&](size_t index) -> DCTDataUnit { return m_transformed_block[index];}, [&](size_t index) -> DCTDataUnit & { return m_ycbcr_block[index]; });

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &
BlockDecompressChain<D, RGBUNIT, QDATAUNIT>::decenterValues() {
  for (size_t i = 0; i < constpow(8, D); i++) {
    m_ycbcr_block[i] += (static_cast<RGBUNIT>(-1)/2)+1;
  }

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &
BlockDecompressChain<D, RGBUNIT, QDATAUNIT>::colorConvert(void (*f)(RGBPixel<double> &, YCbCrUnit)) {
  for (size_t i = 0; i < constpow(8, D); i++) {
    f(m_rgb_block[i], m_ycbcr_block[i]);
  }

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &
BlockDecompressChain<D, RGBUNIT, QDATAUNIT>::putRGBBlock(RGBUNIT *rgb_data, const uint64_t img_dims[D], size_t block) {
  auto inputR = [&](size_t index) {
    return std::clamp<double>(m_rgb_block[index].r, 0, static_cast<RGBUNIT>(-1));
  };

  auto inputG = [&](size_t index) {
    return std::clamp<double>(m_rgb_block[index].g, 0, static_cast<RGBUNIT>(-1));
  };

  auto inputB = [&](size_t index) {
    return std::clamp<double>(m_rgb_block[index].b, 0, static_cast<RGBUNIT>(-1));
  };


  auto outputR = [&](size_t index) -> RGBUNIT & {
    return rgb_data[index * 3 + 0];
  };

  auto outputG = [&](size_t index) -> RGBUNIT & {
    return rgb_data[index * 3 + 1];
  };

  auto outputB = [&](size_t index) -> RGBUNIT & {
    return rgb_data[index * 3 + 2];
  };

  putBlock<D, RGBUNIT>(inputR, block, img_dims, outputR);
  putBlock<D, RGBUNIT>(inputG, block, img_dims, outputG);
  putBlock<D, RGBUNIT>(inputB, block, img_dims, outputB);

  return *this;
}
