#include "block_compress_chain.h"

#include "block.h"

template class BlockCompressChain<2, uint8_t, int16_t>;
template class BlockCompressChain<3, uint8_t, int16_t>;
template class BlockCompressChain<4, uint8_t, int16_t>;

template class BlockCompressChain<2, uint16_t, int32_t>;
template class BlockCompressChain<3, uint16_t, int32_t>;
template class BlockCompressChain<4, uint16_t, int32_t>;

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockCompressChain<D, RGBUNIT, QDATAUNIT> &
BlockCompressChain<D, RGBUNIT, QDATAUNIT>::newRGBBlock(const RGBUNIT *rgb_data, const uint64_t img_dims[D], size_t block) {
  auto inputR = [&](size_t index) {
    return rgb_data[index * 3 + 0];
  };

  auto inputG = [&](size_t index) {
    return rgb_data[index * 3 + 1];
  };

  auto inputB = [&](size_t index) {
    return rgb_data[index * 3 + 2];
  };


  auto outputR = [&](size_t index) -> RGBUNIT & {
    return m_rgb_block[index].r;
  };

  auto outputG = [&](size_t index) -> RGBUNIT & {
    return m_rgb_block[index].g;
  };

  auto outputB = [&](size_t index) -> RGBUNIT & {
    return m_rgb_block[index].b;
  };

  getBlock<D, RGBUNIT>(inputR, block, img_dims, outputR);
  getBlock<D, RGBUNIT>(inputG, block, img_dims, outputG);
  getBlock<D, RGBUNIT>(inputB, block, img_dims, outputB);

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockCompressChain<D, RGBUNIT, QDATAUNIT> &
BlockCompressChain<D, RGBUNIT, QDATAUNIT>::colorConvert(YCbCrUnit (*f)(double, double, double)) {
  for (size_t i = 0; i < constpow(8, D); i++) {
    double R = m_rgb_block[i].r;
    double G = m_rgb_block[i].g;
    double B = m_rgb_block[i].b;

    m_ycbcr_block[i] = f(R, G, B);
  }

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockCompressChain<D, RGBUNIT, QDATAUNIT> &
BlockCompressChain<D, RGBUNIT, QDATAUNIT>::centerValues() {
  for (size_t i = 0; i < constpow(8, D); i++) {
    m_ycbcr_block[i] -= (static_cast<RGBUNIT>(-1)/2)+1;
  }

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockCompressChain<D, RGBUNIT, QDATAUNIT> &
BlockCompressChain<D, RGBUNIT, QDATAUNIT>::forwardDiscreteCosineTransform() {
  m_transformed_block.fill(0);

  fdct<D>([&](size_t index) -> DCTDataUnit { return m_ycbcr_block[index];}, [&](size_t index) -> DCTDataUnit & { return m_transformed_block[index]; });

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockCompressChain<D, RGBUNIT, QDATAUNIT> &
BlockCompressChain<D, RGBUNIT, QDATAUNIT>::quantize(const QuantTable<D, RGBUNIT> &quant_table) {
  for (size_t i = 0; i < constpow(8, D); i++) {
    m_quantized_block[i] = m_transformed_block[i] / quant_table[i];
  }

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockCompressChain<D, RGBUNIT, QDATAUNIT> &
BlockCompressChain<D, RGBUNIT, QDATAUNIT>::addToReferenceBlock(ReferenceBlock<D> &reference) {
  for (size_t i = 0; i < constpow(8, D); i++) {
    reference[i] += abs(m_quantized_block[i]);
  }

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockCompressChain<D, RGBUNIT, QDATAUNIT> &
BlockCompressChain<D, RGBUNIT, QDATAUNIT>::diffEncodeDC(QDATAUNIT &previous_DC) {
  QDATAUNIT current_DC = m_quantized_block[0];
  m_quantized_block[0] -= previous_DC;
  previous_DC = current_DC;

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockCompressChain<D, RGBUNIT, QDATAUNIT> &
BlockCompressChain<D, RGBUNIT, QDATAUNIT>::traverse(const TraversalTable<D> &traversal_table) {
  for (size_t i = 0; i < constpow(8, D); i++) {
    m_traversed_block[traversal_table[i]] = m_quantized_block[i];
  }

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockCompressChain<D, RGBUNIT, QDATAUNIT> &
BlockCompressChain<D, RGBUNIT, QDATAUNIT>::runLengthEncode() {
  std::vector<RunLengthPair<QDATAUNIT>> runlength {};

  runlength.push_back({0, m_traversed_block[0]});

  size_t maxzeroes = constpow(2, RunLengthPair<QDATAUNIT>::zeroesBits());

  size_t zeroes = 0;
  for (size_t i = 1; i < constpow(8, D); i++) {
    if (m_traversed_block[i] == 0) {
      zeroes++;
    }
    else {
      while (zeroes >= maxzeroes) {
        runlength.push_back({maxzeroes - 1, 0});
        zeroes -= maxzeroes;
      }
      runlength.push_back({zeroes, m_traversed_block[i]});
      zeroes = 0;
    }
  }

  runlength.push_back({0, 0});

  m_runlength = runlength;

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockCompressChain<D, RGBUNIT, QDATAUNIT> &
BlockCompressChain<D, RGBUNIT, QDATAUNIT>::huffmanAddWeights(HuffmanWeights weights[2]) {
  m_runlength[0].addToWeights(weights[0]);
  for (size_t i = 1; i < m_runlength.size(); i++) {
    m_runlength[i].addToWeights(weights[1]);
  }

  return *this;
}

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
BlockCompressChain<D, RGBUNIT, QDATAUNIT> &
BlockCompressChain<D, RGBUNIT, QDATAUNIT>::encodeToStream(HuffmanEncoder encoder[2], OBitstream &stream) {
  m_runlength[0].huffmanEncodeToStream(encoder[0], stream);
  for (size_t i = 1; i < m_runlength.size(); i++) {
    m_runlength[i].huffmanEncodeToStream(encoder[1], stream);
  }

  return *this;
}
