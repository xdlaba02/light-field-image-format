/******************************************************************************\
* SOUBOR: lfif_encoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_ENCODER_H
#define LFIF_ENCODER_H

#include "block_compress_chain.h"
#include "colorspace.h"

#include <fstream>

using namespace std;

template<size_t D>
int LFIFCompress(const RGBDataUnit *rgb_data, const uint64_t img_dims[D+1], uint8_t quality, const char *output_file_name) {
  BlockCompressChain<D> block_compress_chain     {};
  QuantTable<D>         quant_table[2]           {};
  ReferenceBlock<D>     reference_block[2]       {};
  TraversalTable<D>     traversal_table[2]       {};
  QuantizedDataUnit     previous_DC[3]           {};
  HuffmanWeights        huffman_weight[2][2]     {};
  HuffmanCodelengths    huffman_codelength[2][2] {};
  HuffmanMap            huffman_map[2][2]        {};

  YCbCrUnit        (*color_convertors[3]) (RGBDataUnit, RGBDataUnit, RGBDataUnit) {};
  QuantTable<D>     *quant_tables[3]     {};
  ReferenceBlock<D> *reference_blocks[3] {};
  TraversalTable<D> *traversal_tables[3] {};
  HuffmanWeights    *huffman_weights[3]  {};
  HuffmanMap        *huffman_maps[3]     {};

  size_t blocks_cnt {};
  size_t pixels_cnt {};

  color_convertors[0] = RGBToY;
  color_convertors[1] = RGBToCb;
  color_convertors[2] = RGBToCr;

  quant_tables[0] = &quant_table[0];
  quant_tables[1] = &quant_table[1];
  quant_tables[2] = &quant_table[1];

  reference_blocks[0] = &reference_block[0];
  reference_blocks[1] = &reference_block[1];
  reference_blocks[2] = &reference_block[1];

  traversal_tables[0] = &traversal_table[0];
  traversal_tables[1] = &traversal_table[1];
  traversal_tables[2] = &traversal_table[1];

  huffman_weights[0] = huffman_weight[0];
  huffman_weights[1] = huffman_weight[1];
  huffman_weights[2] = huffman_weight[1];

  huffman_maps[0] = huffman_map[0];
  huffman_maps[1] = huffman_map[1];
  huffman_maps[2] = huffman_map[1];

  blocks_cnt = 1;
  pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    blocks_cnt *= ceil(img_dims[i]/8.);
    pixels_cnt *= img_dims[i];
  }

  quant_table[0]
  . baseLuma();

  quant_table[1]
  . baseChroma();

  for (size_t i = 0; i < 2; i++) {
    quant_table[i]
    . scaleByQuality(quality);
  }

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
      huffman_codelength[y][x] = generateHuffmanCodelengths(huffman_weights[y][x]);
      huffman_map[y][x]        = generateHuffmanMap(huffman_codelength[y][x]);
    }
  }

  size_t last_slash_pos = string(output_file_name).find_last_of('/');

  if (last_slash_pos != string::npos) {
    string command("mkdir -p " + string(output_file_name).substr(0, last_slash_pos));
    system(command.c_str());
  }

  ofstream output(output_file_name);
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
      writeHuffmanTable(huffman_codelength[y][x], output);
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
        . encodeToStream(huffman_maps[channel], bitstream);
      }
    }
  }

  bitstream.flush();

  return 0;
}

#endif
