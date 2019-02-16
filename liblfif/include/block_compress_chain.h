/******************************************************************************\
* SOUBOR: block_compress_chain.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef BLOCK_COMPRESS_CHAIN
#define BLOCK_COMPRESS_CHAIN

#include "lfiftypes.h"
#include "block.h"
#include "quant_table.h"
#include "traversal_table.h"
#include "bitstream.h"
#include "dct.h"
#include "huffman.h"

#include <cstdlib>
#include <cstdint>

template <size_t D>
class BlockCompressChain {
public:
  BlockCompressChain<D> &newRGBBlock(const RGBDataUnit *rgb_data, const uint64_t img_dims[D], size_t block) {
    auto inputR = [&](size_t index) {
      return rgb_data[index * 3 + 0];
    };

    auto inputG = [&](size_t index) {
      return rgb_data[index * 3 + 1];
    };

    auto inputB = [&](size_t index) {
      return rgb_data[index * 3 + 2];
    };


    auto outputR = [&](size_t index) -> RGBDataUnit & {
      return m_rgb_block[index].r;
    };

    auto outputG = [&](size_t index) -> RGBDataUnit & {
      return m_rgb_block[index].g;
    };

    auto outputB = [&](size_t index) -> RGBDataUnit & {
      return m_rgb_block[index].b;
    };

    getBlock<D>(inputR, block, img_dims, outputR);
    getBlock<D>(inputG, block, img_dims, outputG);
    getBlock<D>(inputB, block, img_dims, outputB);

    return *this;
  }

  BlockCompressChain<D> &colorConvert(YCbCrUnit (*f)(RGBDataUnit, RGBDataUnit, RGBDataUnit)) {
    for (size_t i = 0; i < constpow(8, D); i++) {
      RGBDataUnit R = m_rgb_block[i].r;
      RGBDataUnit G = m_rgb_block[i].g;
      RGBDataUnit B = m_rgb_block[i].b;

      m_ycbcr_block[i] = f(R, G, B);
    }

    return *this;
  }

  BlockCompressChain<D> &centerValues() {
    for (size_t i = 0; i < constpow(8, D); i++) {
      m_ycbcr_block[i] -= 128;
    }

    return *this;
  }

  BlockCompressChain<D> &forwardDiscreteCosineTransform() {
    fdct<D>([&](size_t index) -> DCTDataUnit { return m_ycbcr_block[index];}, [&](size_t index) -> DCTDataUnit & { return m_transformed_block[index]; });

    return *this;
  }

  BlockCompressChain<D> &quantize(const QuantTable<D> &quant_table) {
    for (size_t i = 0; i < constpow(8, D); i++) {
      m_quantized_block[i] = m_transformed_block[i] / quant_table[i];
    }

    return *this;
  }

  BlockCompressChain<D> &addToReferenceBlock(ReferenceBlock<D> &reference) {
    for (size_t i = 0; i < constpow(8, D); i++) {
      reference[i] += abs(m_quantized_block[i]);
    }

    return *this;
  }

  BlockCompressChain<D> &diffEncodeDC(QuantizedDataUnit &previous_DC) {
    QuantizedDataUnit current_DC = m_quantized_block[0];
    m_quantized_block[0] -= previous_DC;
    previous_DC = current_DC;

    return *this;
  }

  BlockCompressChain<D> &traverse(const TraversalTable<D> &traversal_table) {
    for (size_t i = 0; i < constpow(8, D); i++) {
      m_traversed_block[traversal_table[i]] = m_quantized_block[i];
    }

    return *this;
  }

  BlockCompressChain<D> &runLengthEncode() {
    m_runlength.clear();

    m_runlength.push_back({0, static_cast<QuantizedDataUnit>(m_traversed_block[0])});

    size_t zeroes = 0;
    for (size_t i = 1; i < constpow(8, D); i++) {
      if (m_traversed_block[i] == 0) {
        zeroes++;
      }
      else {
        while (zeroes >= 16) {
          m_runlength.push_back({15, 0});
          zeroes -= 16;
        }
        m_runlength.push_back({static_cast<RunLengthZeroesCountUnit>(zeroes), m_traversed_block[i]});
        zeroes = 0;
      }
    }

    m_runlength.push_back({0, 0});

    return *this;
  }

  BlockCompressChain<D> &huffmanAddWeights(HuffmanWeights weights[2]) {
    weights[0][m_runlength[0].huffmanSymbol()]++;
    for (size_t i = 1; i < m_runlength.size(); i++) {
      weights[1][m_runlength[i].huffmanSymbol()]++;
    }

    return *this;
  }

  BlockCompressChain<D> &encodeToStream(const HuffmanMap map[2], OBitstream &stream) {
    m_runlength[0].huffmanEncodeToStream(map[0], stream);
    for (size_t i = 1; i < m_runlength.size(); i++) {
      m_runlength[i].huffmanEncodeToStream(map[1], stream);
    }

    return *this;
  }

private:
  Block<RGBPixel, D> m_rgb_block;
  Block<YCbCrUnit, D> m_ycbcr_block;
  Block<DCTDataUnit, D> m_transformed_block;
  Block<QuantizedDataUnit, D> m_quantized_block;
  Block<QuantizedDataUnit, D> m_traversed_block;
  vector<RunLengthPair> m_runlength;
};

#endif
