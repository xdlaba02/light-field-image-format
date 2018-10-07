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
  uint64_t blockCount();
  uint64_t pixelCount();

  void huffmanDecode();
  void runLengthDecodeAC();
  static void runLengthDecodeACBlock(const std::vector<RunLengthPair> &input, Block<int8_t> &output);
  void diffDecodeDC();
  void dezigzag();
  void dequantize();
  void backwardDCT();
  static void backwardDCTBlock(const Block<int8_t> &input, Block<double> &output);
  void deblockize();
  void YCbCrToRGB();

  static RunLengthPair decodeValue(const HuffmanDecoder &decoder, std::vector<bool>::iterator &it);
  static int8_t decodeAmplitude(const uint8_t len, std::vector<bool>::iterator &it);

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

  std::vector<Block<int8_t>> m_channel_Y_run_length_decoded;
  std::vector<Block<int8_t>> m_channel_Cb_run_length_decoded;
  std::vector<Block<int8_t>> m_channel_Cr_run_length_decoded;

  std::vector<Block<int8_t>> m_channel_Y_unzigzaged;
  std::vector<Block<int8_t>> m_channel_Cb_unzigzaged;
  std::vector<Block<int8_t>> m_channel_Cr_unzigzaged;

  std::vector<Block<int8_t>> m_channel_Y_dequantized;
  std::vector<Block<int8_t>> m_channel_Cb_dequantized;
  std::vector<Block<int8_t>> m_channel_Cr_dequantized;

  std::vector<Block<double>> m_channel_Y_detransformed;
  std::vector<Block<double>> m_channel_Cb_detransformed;
  std::vector<Block<double>> m_channel_Cr_detransformed;

  std::vector<uint8_t> m_channel_Y_deblockized;
  std::vector<uint8_t> m_channel_Cb_deblockized;
  std::vector<uint8_t> m_channel_Cr_deblockized;

  std::vector<uint8_t> m_channel_R;
  std::vector<uint8_t> m_channel_G;
  std::vector<uint8_t> m_channel_B;
};

#endif
