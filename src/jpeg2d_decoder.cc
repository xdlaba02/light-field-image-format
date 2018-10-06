#include "jpeg2d_decoder.h"

#include <cstring>
#include <fstream>
#include <bitset>
#include <iostream>

JPEG2DDecoder::JPEG2DDecoder():
  m_width(),
  m_height(),
  m_quant_table_luma(),
  m_quant_table_chroma(),
  m_huffman_decoder_luma_DC(),
  m_huffman_decoder_luma_AC(),
  m_huffman_decoder_chroma_DC(),
  m_huffman_decoder_chroma_AC(),
  m_jpeg2d_data(),
  m_channel_Y_DC(),
  m_channel_Cb_DC(),
  m_channel_Cr_DC(),
  m_channel_Y_AC(),
  m_channel_Cb_AC(),
  m_channel_Cr_AC(),
  m_channel_Y_zigzag(),
  m_channel_Cb_zigzag(),
  m_channel_Cr_zigzag(),
  m_channel_Y_quantized(),
  m_channel_Cb_quantized(),
  m_channel_Cr_quantized(),
  m_channel_Y_transformed(),
  m_channel_Cb_transformed(),
  m_channel_Cr_transformed(),
  m_channel_Y_blocky(),
  m_channel_Cb_blocky(),
  m_channel_Cr_blocky(),
  m_channel_Y_raw(),
  m_channel_Cb_raw(),
  m_channel_Cr_raw(),
  m_channel_R(),
  m_channel_G(),
  m_channel_B() {}

JPEG2DDecoder::~JPEG2DDecoder() {}

bool JPEG2DDecoder::load(const std::string filename) {
  std::ifstream input(filename);
  if (input.fail()) {
    return false;
  }

  char magic_number[8];
  input.read(magic_number, 8);
  if (strncmp(magic_number, "JPEG-2D\n", 8) != 0) {
    return false;
  }

  uint64_t swapped_w;
  uint64_t swapped_h;

  input.read(reinterpret_cast<char *>(&swapped_w), 8);
  input.read(reinterpret_cast<char *>(&swapped_h), 8);

  m_width = fromBigEndian(swapped_w);
  m_height = fromBigEndian(swapped_h);

  input.read(reinterpret_cast<char *>(m_quant_table_luma), 64);
  input.read(reinterpret_cast<char *>(m_quant_table_chroma), 64);

  m_huffman_decoder_luma_DC.load(input);
  m_huffman_decoder_luma_AC.load(input);
  m_huffman_decoder_luma_AC.print();
  m_huffman_decoder_chroma_DC.load(input);
  m_huffman_decoder_chroma_AC.load(input);

  return true;

  uint8_t buff;
  input.read(reinterpret_cast<char *>(&buff), 1);

  while (input.good()) {
    std::bitset<8> bits(buff);
    for (uint8_t i = 0; i < 8; i++) {
      m_jpeg2d_data.push_back(bits[i]);
    }
  }

  return true;
}
