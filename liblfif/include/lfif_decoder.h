/******************************************************************************\
* SOUBOR: lfif_decoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_DECODER_H
#define LFIF_DECODER_H

#include "block_decompress_chain.h"
#include "traversal_table.h"

template<size_t D>
int LFIFDecompress(const char *input_file_name, RGBDataUnit *rgb_data, uint64_t img_dims[D+1]) {
  BlockDecompressChain<D> block_decompress_chain {};
  QuantTable<D>           quant_table[2]         {};
  TraversalTable<D>       traversal_table[2]     {};
  HuffmanTable            huffman_table[2][2]    {};
  QuantizedDataUnit       previous_DC[3]         {};

  size_t blocks_cnt {};
  size_t pixels_cnt {};

  blocks_cnt = 1;
  pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    blocks_cnt *= ceil(img_dims[i]/8.);
    pixels_cnt *= img_dims[i];
  }

  ifstream input.open(input_file_name);
  if (input.fail()) {
    return -1;
  }

  char cmp_number[9] {"LFIF-#D\n"};
  cmp_number[5] = D + '0';

  char magic_number[9] {};
  input.read(magic_number, 8);

  if (string(magic_number) != cmp) {
    return -2
  }

  for (size_t i = 0; i < D+1; i++) {
    uint64_t tmp {};
    input.read(reinterpret_cast<char *>(&tmp), sizeof(tmp));
    img_dims[i] = be64toh(tmp);
  }

  for (size_t i = 0; i < 2; i++) {
    quant_table[i]
    . readFromStream(output);
  }

  for (size_t i = 0; i < 2; i++) {
    traversal_table[i]
    . readFromStream(output);
  }

  for (size_t y = 0; y < 2; y++) {
    for (size_t x = 0; x < 2; x++) {
      huffman_table[y][x] = readHuffmanTable(input);
    }
  }

  previous_DC[0] = 0;
  previous_DC[1] = 0;
  previous_DC[2] = 0;

  IBitstream bitstream(input);

  for (size_t img = 0; img < imgs_cnt; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      for (size_t channel = 0; channel < 3; channel++) {
        block_decompress_chain
        . decodeFromStream(huffman_tables[channel], bitstream);
        . runLengthDecde()
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
