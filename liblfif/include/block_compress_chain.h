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
#include "runlength.h"
#include "block.h"

#include <cstdlib>
#include <cstdint>

template <size_t D>
class BlockCompressChain {
public:
  BlockCompressChain<D> &newRGBBlock(const RGBUnit8 *rgb_data, const uint64_t img_dims[D], size_t block) {
    auto inputR = [&](size_t index) {
      return rgb_data[index * 3 + 0];
    };

    auto inputG = [&](size_t index) {
      return rgb_data[index * 3 + 1];
    };

    auto inputB = [&](size_t index) {
      return rgb_data[index * 3 + 2];
    };


    auto outputR = [&](size_t index) -> RGBUnit8 & {
      return m_rgb_block[index].r;
    };

    auto outputG = [&](size_t index) -> RGBUnit8 & {
      return m_rgb_block[index].g;
    };

    auto outputB = [&](size_t index) -> RGBUnit8 & {
      return m_rgb_block[index].b;
    };

    getBlock<D, RGBUnit8>(inputR, block, img_dims, outputR);
    getBlock<D, RGBUnit8>(inputG, block, img_dims, outputG);
    getBlock<D, RGBUnit8>(inputB, block, img_dims, outputB);

    return *this;
  }

  BlockCompressChain<D> &colorConvert(YCbCrUnit8 (*f)(RGBUnit8, RGBUnit8, RGBUnit8)) {
    for (size_t i = 0; i < constpow(8, D); i++) {
      RGBUnit8 R = m_rgb_block[i].r;
      RGBUnit8 G = m_rgb_block[i].g;
      RGBUnit8 B = m_rgb_block[i].b;

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
    m_transformed_block.fill(0);

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

  BlockCompressChain<D> &diffEncodeDC(QuantizedDataUnit8 &previous_DC) {
    QuantizedDataUnit8 current_DC = m_quantized_block[0];
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
    std::vector<RunLengthPair> runlength {};

    runlength.push_back({0, m_traversed_block[0]});

    RunLengthZeroesCountUnit zeroes = 0;
    for (size_t i = 1; i < constpow(8, D); i++) {
      if (m_traversed_block[i] == 0) {
        zeroes++;
      }
      else {
        while (zeroes >= 16) {
          runlength.push_back({15, 0});
          zeroes -= 16;
        }
        runlength.push_back({zeroes, m_traversed_block[i]});
        zeroes = 0;
      }
    }

    runlength.push_back({0, 0});

    m_runlength = runlength;

    return *this;
  }

  BlockCompressChain<D> &huffmanAddWeights(HuffmanWeights weights[2]) {
    m_runlength[0].addToWeights(weights[0]);
    for (size_t i = 1; i < m_runlength.size(); i++) {
      m_runlength[i].addToWeights(weights[1]);
    }

    return *this;
  }
  BlockCompressChain<D> &encodeToStream(HuffmanEncoder encoder[2], OBitstream &stream) {
    m_runlength[0].huffmanEncodeToStream(encoder[0], stream);
    for (size_t i = 1; i < m_runlength.size(); i++) {
      m_runlength[i].huffmanEncodeToStream(encoder[1], stream);
    }

    return *this;
  }

private:
  Block<RGBPixel<RGBUnit8>, D> m_rgb_block;
  Block<YCbCrUnit8, D>         m_ycbcr_block;
  Block<DCTDataUnit, D>        m_transformed_block;
  Block<QuantizedDataUnit8, D> m_quantized_block;
  Block<QuantizedDataUnit8, D> m_traversed_block;
  std::vector<RunLengthPair>   m_runlength;
};

#endif
