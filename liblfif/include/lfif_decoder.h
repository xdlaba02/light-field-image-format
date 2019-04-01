/******************************************************************************\
* SOUBOR: lfif_decoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_DECODER_H
#define LFIF_DECODER_H

#include <cstdint>

#include <iosfwd>

template<size_t D, typename T>
int LFIFDecompress(istream &input, const uint64_t img_dims[D+1], T max_rgb_value, T *rgb_data) {
  BlockDecompressChain<D, T> block_decompress_chain {};
  QuantTable<D>              quant_table     [2]    {};
  TraversalTable<D>          traversal_table [2]    {};
  HuffmanDecoder             huffman_decoder [2][2] {};

  QDATAUNIT                  previous_DC     [3]    {};

  HuffmanDecoder            *huffman_decoders[3]    {};
  TraversalTable<D>         *traversal_tables[3]    {};
  QuantTable<D>             *quant_tables    [3]    {};
  void                     (*color_convertors[3])
          (RGBPixel<double> &, YCBCRUNIT, uint16_t) {};

  size_t blocks_cnt {};
  size_t pixels_cnt {};

  size_t rgb_bits    {};
  size_t amp_bits    {};
  size_t class_bits  {};

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
  }

  for (size_t y = 0; y < 2; y++) {
    for (size_t x = 0; x < 2; x++) {
      huffman_decoder[y][x]
      . readFromStream(input);
    }
  }

  blocks_cnt = 1;
  pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    blocks_cnt *= ceil(img_dims[i]/static_cast<double>(BLOCK_SIZE));
    pixels_cnt *= img_dims[i];
  }

  rgb_bits = ceil(log2(max_rgb_value));
  amp_bits = ceil(log2(constpow(BLOCK_SIZE, D))) + rgb_bits - D - (D/2);
  class_bits = RunLengthPair::classBits(amp_bits);

  previous_DC[0] = 0;
  previous_DC[1] = 0;
  previous_DC[2] = 0;

  IBitstream bitstream(&input);

  for (size_t img = 0; img < img_dims[D]; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      for (size_t channel = 0; channel < 3; channel++) {
        block_decompress_chain
        . decodeFromStream(huffman_decoders[channel], bitstream, class_bits)
        . runLengthDecode()
        . detraverse(*traversal_tables[channel])
        . diffDecodeDC(previous_DC[channel])
        . dequantize(*quant_tables[channel])
        . inverseDiscreteCosineTransform()
        . decenterValues(max_rgb_value)
        . colorConvert(color_convertors[channel], max_rgb_value);
      }

      block_decompress_chain
      . putRGBBlock(&rgb_data[img * pixels_cnt * 3], img_dims, block, max_rgb_value);
    }
  }

  return 0;
}


#endif
