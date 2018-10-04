#ifndef JPEG2D_H
#define JPEG2D_H

#include "huffman.h"
#include "bitstream.h"

#include <cstdint>

struct JPEG2D {
  uint64_t width;
  uint64_t height;

  Block<uint8_t> quant_table_luma;
  Block<uint8_t> quant_table_chroma;

  HuffmanTable huffman_luma_DC;
  HuffmanTable huffman_luma_AC;
  HuffmanTable huffman_chroma_AC;
  HuffmanTable huffman_chroma_DC;

  Bitstream data;
};

#endif
