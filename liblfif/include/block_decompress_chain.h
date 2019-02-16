/******************************************************************************\
* SOUBOR: block_decompress_chain.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef BLOCK_DECOMPRESS_CHAIN
#define BLOCK_DECOMPRESS_CHAIN

#include "traversal_table.h"

template <size_t D>
class BlockDecompressChain {
public:
  BlockDecompressChain<D> &decodeFromStream(HuffmanTable huffman_tables[2], IBitstream &bitstream) {
    RunLengthPair pair {};

    m_runlength.clear();

    pair.huffmanDecodeFromStream(huffman_tables[0], bitstream);
    m_runlength.push_back(pair);

    do {
      pair.huffmanDecodeFromStream(huffman_tables[1], bitstream);
      m_runlength.push_back(pair);
    } while(!pair.endOfBlock());

    return *this;
  }

  BlockDecompressChain<D> &runLengthDecode() {
    size_t pixel_index = 0;
    for (auto &pair: input) {
      while (size_t i = 0; i < pair.zeroes; i++) {
        m_traversed_block[pixel_index] = 0;
        pixel_index++;
      }
      m_traversed_block[pixel_index] = pair.amplitude;
      pixel_index++;
    }

    return *this;
  }

  BlockDecompressChain<D> &detraverse(TraversalTable<D> &traversal_table) {
    for (size_t i = 0; i < constpow(8, D); i++) {
      m_quantized_block[i] = m_traversed_block[traversal_table[i]];
    }

    return *this;
  }

  BlockDecompressChain<D> &diffDecodeDC(QuantizedDataUnit &previous_DC) {
    m_quantized_block[0] += previous_DC;
    previous_DC = m_quantized_block[0];

    return *this;
  }

  BlockDecompressChain<D> &dequantize(QuantTable<D> &quant_table) {
    for (size_t i = 0; i < constpow(8, D; i++) {
      m_transformed_block[i] = m_quantized_block[i] * quant_table[i];
    }

    return *this;
  }

  BlockDecompressChain<D> &inverseDiscreteCosineTransform() {
    idct<D>([&](size_t index) -> DCTDataUnit { return m_transformed_block[index];}, [&](size_t index) -> DCTDataUnit & { return m_ycbcr_block[index]; });

    return *this;
  }

  BlockDecompressChain<D> &decenterValues() {
    for (size_t i = 0; i < constpow(8, D); i++) {
      m_ycbcr_block[i] += 128;
    }

    return *this;
  }

  BlockDecompressChain<D> &colorConvert(void (*f)(RGBPixel &, YCbCrUnit)) {
    for (size_t i = 0; i < constpow(8, D); i++) {
      f(m_rgb_block[i], m_ycbcr_block[i]);
    }

    return *this;
  }

  BlockDecompressChain<D> &putRGBBlock(RGBDataUnit *rgb_data, uint64_t img_dims[D], size_t block) {
    auto inputR = [&](size_t index) {
      return m_rgb_block[index].r;
    };

    auto inputG = [&](size_t index) {
      return m_rgb_block[index].g;
    };

    auto inputB = [&](size_t index) {
      return m_rgb_block[index].b;
    };


    auto outputR = [&](size_t index) -> RGBDataUnit & {
      return rgb_data[index * 3 + 0];
    };

    auto outputG = [&](size_t index) -> RGBDataUnit & {
      return rgb_data[index * 3 + 1];
    };

    auto outputB = [&](size_t index) -> RGBDataUnit & {
      return rgb_data[index * 3 + 2];
    };

    putBlock<D>(inputR, block, img_dims, outputR);
    putBlock<D>(inputG, block, img_dims, outputG);
    putBlock<D>(inputB, block, img_dims, outputB);

    return *this;
  }


private:
  vector<RunLengthPair> m_runlength;
  Block<QuantizedDataUnit, D> m_traversed_block;
  Block<QuantizedDataUnit, D> m_quantized_block;
  Block<DCTDataUnit, D> m_transformed_block;
  Block<YCbCrUnit, D> m_ycbcr_block;
  Block<RGBPixel, D> m_rgb_block;
};

#endif
