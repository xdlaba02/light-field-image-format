/******************************************************************************\
* SOUBOR: lfif_encoder.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "lfif_encoder.h"
#include "colorspace.h"
#include "bitstream.h"
#include "block_compress_chain.h"

#include <fstream>

using namespace std;

template int LFIFCompress<2, uint8_t>(const uint8_t *, const uint64_t *, uint8_t, uint8_t, ofstream &);
template int LFIFCompress<3, uint8_t>(const uint8_t *, const uint64_t *, uint8_t, uint8_t, ofstream &);
template int LFIFCompress<4, uint8_t>(const uint8_t *, const uint64_t *, uint8_t, uint8_t, ofstream &);

template int LFIFCompress<2, uint16_t>(const uint16_t *, const uint64_t *, uint8_t, uint16_t, ofstream &);
template int LFIFCompress<3, uint16_t>(const uint16_t *, const uint64_t *, uint8_t, uint16_t, ofstream &);
template int LFIFCompress<4, uint16_t>(const uint16_t *, const uint64_t *, uint8_t, uint16_t, ofstream &);

template<size_t D, typename T>
int LFIFCompress(const T *rgb_data, const uint64_t img_dims[D+1], uint8_t quality, T max_rgb_value, ofstream &output) {
  BlockCompressChain<D, T> block_compress_chain    {};
  QuantTable<D>            quant_table      [2]    {};
  ReferenceBlock<D>        reference_block  [2]    {};
  TraversalTable<D>        traversal_table  [2]    {};
  HuffmanWeights           huffman_weight   [2][2] {};
  HuffmanEncoder           huffman_encoder  [2][2] {};

  QDATAUNIT                previous_DC      [3]    {};
  YCBCRUNIT              (*color_convertors [3])
                (double, double, double, uint16_t) {};
  QuantTable<D>           *quant_tables    [3]     {};
  ReferenceBlock<D>       *reference_blocks[3]     {};
  TraversalTable<D>       *traversal_tables[3]     {};
  HuffmanWeights          *huffman_weights [3]     {};
  HuffmanEncoder          *huffman_encoders[3]     {};

  size_t blocks_cnt {};
  size_t pixels_cnt {};

  size_t rgb_bits    {};
  size_t amp_bits    {};
  size_t zeroes_bits {};
  size_t class_bits  {};
  size_t max_zeroes  {};

  color_convertors[0] =  RGBToY;
  color_convertors[1] =  RGBToCb;
  color_convertors[2] =  RGBToCr;

  quant_tables[0]     = &quant_table[0];
  quant_tables[1]     = &quant_table[1];
  quant_tables[2]     = &quant_table[1];

  reference_blocks[0] = &reference_block[0];
  reference_blocks[1] = &reference_block[1];
  reference_blocks[2] = &reference_block[1];

  traversal_tables[0] = &traversal_table[0];
  traversal_tables[1] = &traversal_table[1];
  traversal_tables[2] = &traversal_table[1];

  huffman_weights[0]  =  huffman_weight[0];
  huffman_weights[1]  =  huffman_weight[1];
  huffman_weights[2]  =  huffman_weight[1];

  huffman_encoders[0] =  huffman_encoder[0];
  huffman_encoders[1] =  huffman_encoder[1];
  huffman_encoders[2] =  huffman_encoder[1];

  blocks_cnt = 1;
  pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    blocks_cnt *= ceil(img_dims[i]/static_cast<double>(BLOCK_SIZE));
    pixels_cnt *= img_dims[i];
  }

  rgb_bits = ceil(log2(max_rgb_value));
  amp_bits = ceil(log2(constpow(BLOCK_SIZE, D))) + rgb_bits - D - (D/2);
  class_bits = RunLengthPair::classBits(amp_bits);
  zeroes_bits = RunLengthPair::zeroesBits(class_bits);
  max_zeroes = constpow(2, zeroes_bits);

  quant_table[0]
  . baseDiagonalTable(QuantTable<D>::base_luma, quality);

  quant_table[1]
  . baseDiagonalTable(QuantTable<D>::base_chroma, quality);

  for (size_t img = 0; img < img_dims[D]; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      block_compress_chain
      . newRGBBlock(&rgb_data[img * pixels_cnt * 3], img_dims, block);

      for (size_t channel = 0; channel < 3; channel++) {
        block_compress_chain
        . colorConvert(color_convertors[channel], max_rgb_value)
        . centerValues(max_rgb_value)
        . forwardDiscreteCosineTransform()
        . quantize(*quant_tables[channel])
        . addToReferenceBlock(*reference_blocks[channel]);
      }
    }
  }

  for (size_t i = 0; i < 2; i++) {
    traversal_table[i]
    . constructByReference(reference_block[i]);
  }

  previous_DC[0] = 0;
  previous_DC[1] = 0;
  previous_DC[2] = 0;

  for (size_t img = 0; img < img_dims[D]; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      block_compress_chain
      . newRGBBlock(&rgb_data[img * pixels_cnt * 3], img_dims, block);

      for (size_t channel = 0; channel < 3; channel++) {
        block_compress_chain
        . colorConvert(color_convertors[channel], max_rgb_value)
        . centerValues(max_rgb_value)
        . forwardDiscreteCosineTransform()
        . quantize(*quant_tables[channel])
        . diffEncodeDC(previous_DC[channel])
        . traverse(*traversal_tables[channel])
        . runLengthEncode(max_zeroes)
        . huffmanAddWeights(huffman_weights[channel], class_bits);
      }
    }
  }

  for (size_t i = 0; i < 2; i++) {
    quant_table[i]
    . writeToStream(output);
  }

  for (size_t i = 0; i < 2; i++) {
    traversal_table[i]
    . writeToStream(output);
  }

  for (size_t y = 0; y < 2; y++) {
    for (size_t x = 0; x < 2; x++) {
      huffman_encoder[y][x]
      . generateFromWeights(huffman_weight[y][x])
      . writeToStream(output);
    }
  }

  OBitstream bitstream(&output);

  previous_DC[0] = 0;
  previous_DC[1] = 0;
  previous_DC[2] = 0;

  for (size_t img = 0; img < img_dims[D]; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      block_compress_chain
      . newRGBBlock(&rgb_data[img * pixels_cnt * 3], img_dims, block);

      for (size_t channel = 0; channel < 3; channel++) {
        block_compress_chain
        . colorConvert(color_convertors[channel], max_rgb_value)
        . centerValues(max_rgb_value)
        . forwardDiscreteCosineTransform()
        . quantize(*quant_tables[channel])
        . diffEncodeDC(previous_DC[channel])
        . traverse(*traversal_tables[channel])
        . runLengthEncode(max_zeroes)
        . encodeToStream(huffman_encoders[channel], bitstream, class_bits);
      }
    }
  }

  bitstream.flush();

  return 0;
}
