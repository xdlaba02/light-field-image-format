/******************************************************************************\
* SOUBOR: lfif_decoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_DECODER_H
#define LFIF_DECODER_H

#include "block_decompress_chain.h"
#include "bitstream.h"
#include "colorspace.h"

#include <fstream>
#include <vector>

#include <iostream> //FIXME

template<size_t D, typename RGBUNIT, typename QDATAUNIT>
int LFIFDecompress(std::ifstream &input, const uint64_t img_dims[D+1], RGBUNIT *rgb_data) {
  BlockDecompressChain<D, RGBUNIT, QDATAUNIT> block_decompress_chain {};
  QuantTable<D, RGBUNIT>                      quant_table     [2]    {};
  TraversalTable<D>                           traversal_table [2]    {};
  HuffmanDecoder                              huffman_decoder [2][2] {};

  QDATAUNIT                                   previous_DC     [3]    {};

  HuffmanDecoder                             *huffman_decoders[3]    {};
  TraversalTable<D>                          *traversal_tables[3]    {};
  QuantTable<D, RGBUNIT>                     *quant_tables    [3]    {};
  void                                      (*color_convertors[3])
                                     (RGBPixel<double> &, YCbCrUnit) {};

  size_t blocks_cnt {};
  size_t pixels_cnt {};

  huffman_decoders[0] =  huffman_decoder[0];
  huffman_decoders[1] =  huffman_decoder[1];
  huffman_decoders[2] =  huffman_decoder[1];

  traversal_tables[0] = &traversal_table[0];
  traversal_tables[1] = &traversal_table[1];
  traversal_tables[2] = &traversal_table[1];

  quant_tables[0]     = &quant_table[0];
  quant_tables[1]     = &quant_table[1];
  quant_tables[2]     = &quant_table[1];

  color_convertors[0] = YtoRGB;
  color_convertors[1] = CbtoRGB;
  color_convertors[2] = CrtoRGB;

  for (size_t i = 0; i < 2; i++) {
    quant_table[i]
    . readFromStream(input);
  }

  for (size_t i = 0; i < 2; i++) {
    traversal_table[i]
    . readFromStream(input);
    for (size_t y = 0; y < 8; y++) {
      for (size_t x = 0; x < 8; x++) {
        std::cerr << (unsigned long)traversal_table[i][y * 8 + x] << ", ";
      }
      std::cerr << "\n";
    }
    std::cerr << "\n";
  }

  for (size_t y = 0; y < 2; y++) {
    for (size_t x = 0; x < 2; x++) {
      huffman_decoder[y][x]
      . readFromStream(input);

      for (auto &val: huffman_decoder[y][x].m_huffman_counts) {
        std::cerr << unsigned(val) << ", ";
      }
      std::cerr << "\n";
      for (auto &val: huffman_decoder[y][x].m_huffman_symbols) {
        std::cerr << std::bitset<8>(val) << ", ";
      }
      std::cerr << "\n";
    }
  }

  std::cerr << "HELLPP\n";

  blocks_cnt = 1;
  pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    blocks_cnt *= ceil(img_dims[i]/8.);
    pixels_cnt *= img_dims[i];
  }

  previous_DC[0] = 0;
  previous_DC[1] = 0;
  previous_DC[2] = 0;

  IBitstream bitstream(input);

  for (size_t img = 0; img < img_dims[D]; img++) {
    std::cerr << "IMG " << img << std::endl;
    for (size_t block = 0; block < blocks_cnt; block++) {
      std::cerr << "BLOCK " << block << std::endl;
      for (size_t channel = 0; channel < 3; channel++) {
        std::cerr << "CHANNEL " << channel << std::endl;
        block_decompress_chain
        . decodeFromStream(huffman_decoders[channel], bitstream)
        . runLengthDecode()
        . detraverse(*traversal_tables[channel])
        . diffDecodeDC(previous_DC[channel])
        . dequantize(*quant_tables[channel])
        . inverseDiscreteCosineTransform()
        . decenterValues()
        . colorConvert(color_convertors[channel]);
      }

      block_decompress_chain
      . putRGBBlock(&rgb_data[img * pixels_cnt * 3], img_dims, block);
    }
  }

  return 0;
}

#endif
