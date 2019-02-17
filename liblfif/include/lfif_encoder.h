/******************************************************************************\
* SOUBOR: lfif_encoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_ENCODER_H
#define LFIF_ENCODER_H

#include "colorspace.h"
#include "bitstream.h"
#include "block_compress_chain.h"

#include <fstream>

template<size_t D>
int LFIFCompress(const RGBUnit8 *rgb_data, const uint64_t img_dims[D+1], uint8_t quality, const char *output_file_name) {
  BlockCompressChain<D> block_compress_chain   {};
  QuantTable<D>         quant_table     [2]    {};
  ReferenceBlock<D>     reference_block [2]    {};
  TraversalTable<D>     traversal_table [2]    {};
  HuffmanWeights        huffman_weight  [2][2] {};
  HuffmanEncoder        huffman_encoder [2][2] {};

  QuantizedDataUnit8    previous_DC     [3]    {};

  YCbCrUnit8          (*color_convertors[3])
                (RGBUnit8, RGBUnit8, RGBUnit8) {};
  QuantTable<D>        *quant_tables    [3]    {};
  ReferenceBlock<D>    *reference_blocks[3]    {};
  TraversalTable<D>    *traversal_tables[3]    {};
  HuffmanWeights       *huffman_weights [3]    {};
  HuffmanEncoder       *huffman_encoders[3]    {};

  size_t                blocks_cnt             {};
  size_t                pixels_cnt             {};

  std::ofstream         output                 {};
  size_t                last_slash_pos         {};

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
    blocks_cnt *= ceil(img_dims[i]/8.);
    pixels_cnt *= img_dims[i];
  }

  quant_table[0]
  . baseLuma()
  . scaleByQuality(quality);

  quant_table[1]
  . baseChroma()
  . scaleByQuality(quality);

  for (size_t img = 0; img < img_dims[D]; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      block_compress_chain
      . newRGBBlock(&rgb_data[img * pixels_cnt * 3], img_dims, block);

      for (size_t channel = 0; channel < 3; channel++) {
        block_compress_chain
        . colorConvert(color_convertors[channel])
        . centerValues()
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
        . colorConvert(color_convertors[channel])
        . centerValues()
        . forwardDiscreteCosineTransform()
        . quantize(*quant_tables[channel])
        . diffEncodeDC(previous_DC[channel])
        . traverse(*traversal_tables[channel])
        . runLengthEncode()
        . huffmanAddWeights(huffman_weights[channel]);
      }
    }
  }

  for (size_t y = 0; y < 2; y++) {
    for (size_t x = 0; x < 2; x++) {
      huffman_encoder[y][x].generateFromWeights(huffman_weights[y][x]);
    }
  }

  last_slash_pos = std::string(output_file_name).find_last_of('/');
  if (last_slash_pos != std::string::npos) {
    std::string command("mkdir -p " + std::string(output_file_name).substr(0, last_slash_pos));
    system(command.c_str());
  }

  output.open(output_file_name);
  if (output.fail()) {
    return -1;
  }

  char magic_number[9] = "LFIF-#D\n";
  magic_number[5] = D + '0';

  output.write(magic_number, 8);

  for (size_t i = 0; i < D+1; i++) {
    uint64_t tmp = htobe64(img_dims[i]);
    output.write(reinterpret_cast<const char *>(&tmp), sizeof(tmp));
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
      . writeToStream(output);
    }
  }

  OBitstream bitstream(output);

  previous_DC[0] = 0;
  previous_DC[1] = 0;
  previous_DC[2] = 0;

  for (size_t img = 0; img < img_dims[D]; img++) {
    for (size_t block = 0; block < blocks_cnt; block++) {
      block_compress_chain
      . newRGBBlock(&rgb_data[img * pixels_cnt * 3], img_dims, block);

      for (size_t channel = 0; channel < 3; channel++) {
        block_compress_chain
        . colorConvert(color_convertors[channel])
        . centerValues()
        . forwardDiscreteCosineTransform()
        . quantize(*quant_tables[channel])
        . diffEncodeDC(previous_DC[channel])
        . traverse(*traversal_tables[channel])
        . runLengthEncode()
        . encodeToStream(huffman_encoders[channel], bitstream);
      }
    }
  }

  bitstream.flush();

  return 0;
}

#endif
