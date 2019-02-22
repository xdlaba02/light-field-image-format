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

template <size_t D, typename T>
class BlockDecompressChain {
public:
  BlockDecompressChain<D, T> &decodeFromStream(HuffmanDecoder huffman_decoders[2], IBitstream &bitstream, size_t rgb_bits);
  BlockDecompressChain<D, T> &runLengthDecode();
  BlockDecompressChain<D, T> &detraverse(TraversalTable<D> &traversal_table);
  BlockDecompressChain<D, T> &diffDecodeDC(QDATAUNIT &previous_DC);
  BlockDecompressChain<D, T> &dequantize(QuantTable<D> &quant_table);
  BlockDecompressChain<D, T> &inverseDiscreteCosineTransform();
  BlockDecompressChain<D, T> &decenterValues(size_t rgb_bits);
  BlockDecompressChain<D, T> &colorConvert(void (*f)(RGBPixel<double> &, YCBCRUNIT, size_t), size_t rgb_bits);
  BlockDecompressChain<D, T> &putRGBBlock(T *rgb_data, const uint64_t img_dims[D], size_t block);

private:
  std::vector<RunLengthPair> m_runlength;
  Block<QDATAUNIT,        D> m_traversed_block;
  Block<QDATAUNIT,        D> m_quantized_block;
  Block<DCTDATAUNIT,      D> m_transformed_block;
  Block<YCBCRUNIT,        D> m_ycbcr_block;
  Block<RGBPixel<double>, D> m_rgb_block;
};

#endif
