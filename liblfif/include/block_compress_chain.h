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

public: //FIXME
  Block<RGBPixel<T>, D>      m_rgb_block;
  Block<YCBCRUNIT,   D>      m_ycbcr_block;
  Block<DCTDATAUNIT, D>      m_transformed_block;
  Block<QDATAUNIT,   D>      m_quantized_block;
  Block<QDATAUNIT,   D>      m_traversed_block;
  std::vector<RunLengthPair> m_runlength;
};

#endif
