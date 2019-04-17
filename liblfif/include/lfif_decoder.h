/******************************************************************************\
* SOUBOR: lfif_decoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_DECODER_H
#define LFIF_DECODER_H

#include "block_decompress_chain.h"
#include "bitstream.h"

#include <cstdint>
#include <istream>
#include <sstream>

template<size_t BS, size_t D>
struct LfifDecoder {
  uint16_t max_rgb_value;
  uint64_t img_dims[D+1];

  QuantTable<BS, D>       quant_table         [2];
  TraversalTable<BS, D>   traversal_table     [2];
  HuffmanDecoder          huffman_decoder     [2][2];

  HuffmanDecoder         *huffman_decoders_ptr[3];
  TraversalTable<BS, D>  *traversal_table_ptr [3];
  QuantTable<BS, D>      *quant_table_ptr     [3];

  size_t blocks_cnt;
  size_t pixels_cnt;

  size_t input_bits;
  size_t amp_bits;
  size_t class_bits;

  Block<INPUTTRIPLET,  BS, D> current_block;
  Block<RunLengthPair, BS, D> runlength;
  Block<QDATAUNIT,     BS, D> quantized_block;
  Block<DCTDATAUNIT,   BS, D> dct_block;
  Block<INPUTUNIT,     BS, D> output_block;
};

template<size_t BS, size_t D>
int readHeader(LfifDecoder<BS, D> &dec, std::istream &input) {
  std::string       magic_number     {};
  std::stringstream magic_number_cmp {};

  std::string       block_size     {};
  std::stringstream block_size_cmp {};

  magic_number_cmp << "LFIF-" << D << "D";
  block_size_cmp   << BS;

  input >> magic_number;
  input.ignore();
  input >> block_size;
  input.ignore();

  if (magic_number != magic_number_cmp.str()) {
    return -1;
  }

  if (block_size != block_size_cmp.str()) {
    return -2;
  }

  dec.max_rgb_value = readValueFromStream<uint16_t>(input);

  for (size_t i = 0; i < D + 1; i++) {
    dec.img_dims[i] = readValueFromStream<uint64_t>(input);
  }

  for (size_t i = 0; i < 2; i++) {
    dec.quant_table[i] = readFromStream<BS, D>(input);
  }

  for (size_t i = 0; i < 2; i++) {
    dec.traversal_table[i]
    . readFromStream(input);
  }

  for (size_t y = 0; y < 2; y++) {
    for (size_t x = 0; x < 2; x++) {
      dec.huffman_decoder[y][x]
      . readFromStream(input);
    }
  }

  return 0;
}

template<size_t BS, size_t D>
void initDecoder(LfifDecoder<BS, D> &dec) {
  dec.huffman_decoders_ptr[0] = dec.huffman_decoder[0];
  dec.huffman_decoders_ptr[1] = dec.huffman_decoder[1];
  dec.huffman_decoders_ptr[2] = dec.huffman_decoder[1];

  dec.traversal_table_ptr[0]  = &dec.traversal_table[0];
  dec.traversal_table_ptr[1]  = &dec.traversal_table[1];
  dec.traversal_table_ptr[2]  = &dec.traversal_table[1];

  dec.quant_table_ptr[0]      = &dec.quant_table[0];
  dec.quant_table_ptr[1]      = &dec.quant_table[1];
  dec.quant_table_ptr[2]      = &dec.quant_table[1];

  dec.blocks_cnt = 1;
  dec.pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    dec.blocks_cnt *= ceil(dec.img_dims[i]/static_cast<double>(BS));
    dec.pixels_cnt *= dec.img_dims[i];
  }

  dec.input_bits = ceil(log2(dec.max_rgb_value));
  dec.amp_bits = ceil(log2(constpow(BS, D))) + dec.input_bits - D - (D/2) + 1;
  dec.class_bits = RunLengthPair::classBits(dec.amp_bits);
}

template<size_t BS, size_t D, typename F>
void decodeScan(LfifDecoder<BS, D> &dec, std::istream &input, F &&output) {
  IBitstream bitstream       {};
  QDATAUNIT  previous_DC [3] {};

  bitstream.open(&input);

  auto inputF = [&](size_t index) -> const auto & {
    return dec.current_block[index];
  };

  for (size_t img = 0; img < dec.img_dims[D]; img++) {
    auto outputF = [&](size_t index, const auto &value) {
      output(img * dec.pixels_cnt + index, value);
    };

    for (size_t block = 0; block < dec.blocks_cnt; block++) {
      for (size_t channel = 0; channel < 3; channel++) {
                      decodeFromStream<BS, D>(bitstream,            dec.runlength, dec.huffman_decoders_ptr[channel], dec.class_bits);
                       runLengthDecode<BS, D>(dec.runlength,        dec.quantized_block);
                            detraverse<BS, D>(dec.quantized_block, *dec.traversal_table_ptr[channel]);
                          diffDecodeDC<BS, D>(dec.quantized_block,  previous_DC[channel]);
                            dequantize<BS, D>(dec.quantized_block,  dec.dct_block, *dec.quant_table_ptr[channel]);
        inverseDiscreteCosineTransform<BS, D>(dec.dct_block,        dec.output_block);

        for (size_t i = 0; i < constpow(BS, D); i++) {
          dec.current_block[i][channel] = dec.output_block[i];
        }
      }

      putBlock<BS, D>(inputF, block, dec.img_dims, outputF);
    }
  }
}

#endif
