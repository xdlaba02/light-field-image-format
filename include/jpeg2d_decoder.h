#ifndef JPEG2D_DECODER_H
#define JPEG2D_DECODER_H

#include "jpeg2d.h"
#include "huffman_decoder.h"

#include <vector>
#include <string>

class JPEG2DDecoder {
public:
  JPEG2DDecoder();
  ~JPEG2DDecoder();

  bool load(const std::string filename);

  void run(uint64_t &width, uint64_t &height, std::vector<uint8_t> &data);

private:
  uint64_t m_width;
  uint64_t m_height;

  QuantTable m_quant_table_luma;
  QuantTable m_quant_table_chroma;

  HuffmanDecoder m_huffman_decoder_luma_DC;
  HuffmanDecoder m_huffman_decoder_luma_AC;
  HuffmanDecoder m_huffman_decoder_chroma_DC;
  HuffmanDecoder m_huffman_decoder_chroma_AC;

  std::vector<bool> m_jpeg2d_data;

  std::vector<int8_t> m_channel_Y_DC;
  std::vector<int8_t> m_channel_Cb_DC;
  std::vector<int8_t> m_channel_Cr_DC;

  std::vector<std::vector<RunLengthPair>> m_channel_Y_AC;
  std::vector<std::vector<RunLengthPair>> m_channel_Cb_AC;
  std::vector<std::vector<RunLengthPair>> m_channel_Cr_AC;

  std::vector<Block<int8_t>> m_channel_Y_zigzag;
  std::vector<Block<int8_t>> m_channel_Cb_zigzag;
  std::vector<Block<int8_t>> m_channel_Cr_zigzag;

  std::vector<Block<int8_t>> m_channel_Y_quantized;
  std::vector<Block<int8_t>> m_channel_Cb_quantized;
  std::vector<Block<int8_t>> m_channel_Cr_quantized;

  std::vector<Block<double>> m_channel_Y_transformed;
  std::vector<Block<double>> m_channel_Cb_transformed;
  std::vector<Block<double>> m_channel_Cr_transformed;

  std::vector<Block<uint8_t>> m_channel_Y_blocky;
  std::vector<Block<uint8_t>> m_channel_Cb_blocky;
  std::vector<Block<uint8_t>> m_channel_Cr_blocky;

  std::vector<uint8_t> m_channel_Y_raw;
  std::vector<uint8_t> m_channel_Cb_raw;
  std::vector<uint8_t> m_channel_Cr_raw;

  std::vector<uint8_t> m_channel_R;
  std::vector<uint8_t> m_channel_G;
  std::vector<uint8_t> m_channel_B;
};

#endif
