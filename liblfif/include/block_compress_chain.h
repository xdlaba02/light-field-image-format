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

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
class BlockCompressChain {
public:

  BlockCompressChain<D, RGBUNIT, QDATAUNIT> &newRGBBlock(const RGBUNIT *rgb_data, const uint64_t img_dims[D], size_t block);
  BlockCompressChain<D, RGBUNIT, QDATAUNIT> &colorConvert(YCbCrUnit (*f)(double, double, double));
  BlockCompressChain<D, RGBUNIT, QDATAUNIT> &centerValues();
  BlockCompressChain<D, RGBUNIT, QDATAUNIT> &forwardDiscreteCosineTransform();
  BlockCompressChain<D, RGBUNIT, QDATAUNIT> &quantize(const QuantTable<D, RGBUNIT> &quant_table);
  BlockCompressChain<D, RGBUNIT, QDATAUNIT> &addToReferenceBlock(ReferenceBlock<D> &reference);
  BlockCompressChain<D, RGBUNIT, QDATAUNIT> &diffEncodeDC(QDATAUNIT &previous_DC);
  BlockCompressChain<D, RGBUNIT, QDATAUNIT> &traverse(const TraversalTable<D> &traversal_table);
  BlockCompressChain<D, RGBUNIT, QDATAUNIT> &runLengthEncode();
  BlockCompressChain<D, RGBUNIT, QDATAUNIT> &huffmanAddWeights(HuffmanWeights weights[2]);
  BlockCompressChain<D, RGBUNIT, QDATAUNIT> &encodeToStream(HuffmanEncoder encoder[2], OBitstream &stream);

private:
  Block<RGBPixel<RGBUNIT>, D>           m_rgb_block;
  Block<YCbCrUnit, D>                   m_ycbcr_block;
  Block<DCTDataUnit, D>                 m_transformed_block;
  Block<QDATAUNIT, D>                   m_quantized_block;
  Block<QDATAUNIT, D>                   m_traversed_block;
  std::vector<RunLengthPair<QDATAUNIT>> m_runlength;
};

#endif
