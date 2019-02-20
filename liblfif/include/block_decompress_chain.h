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
#include "dct.h"

#include <vector>

template <size_t D, typename RGBUNIT, typename QDATAUNIT>
class BlockDecompressChain {
public:
  BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &decodeFromStream(HuffmanDecoder huffman_decoders[2], IBitstream &bitstream);
  BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &runLengthDecode();
  BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &detraverse(TraversalTable<D> &traversal_table);
  BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &diffDecodeDC(QDATAUNIT &previous_DC);
  BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &dequantize(QuantTable<D, RGBUNIT> &quant_table);
  BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &inverseDiscreteCosineTransform();
  BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &decenterValues();
  BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &colorConvert(void (*f)(RGBPixel<double> &, YCbCrUnit));
  BlockDecompressChain<D, RGBUNIT, QDATAUNIT> &putRGBBlock(RGBUNIT *rgb_data, const uint64_t img_dims[D], size_t block);

private:
  std::vector<RunLengthPair<QDATAUNIT>> m_runlength;
  Block<QDATAUNIT, D>                   m_traversed_block;
  Block<QDATAUNIT, D>                   m_quantized_block;
  Block<DCTDataUnit, D>                 m_transformed_block;
  Block<YCbCrUnit, D>                   m_ycbcr_block;
  Block<RGBPixel<double>, D>            m_rgb_block;
};

#endif
