#ifndef JPEG2D_ENCODER_H
#define JPEG2D_ENCODER_H

#include "jpeg2d.h"
#include "huffman_encoder.h"

#include <vector>
#include <string>

class JPEG2DEncoder {
public:
  JPEG2DEncoder(const uint64_t width, const uint64_t height, const uint8_t rgb_data[], const uint8_t quality);
  ~JPEG2DEncoder();

  void run();

  bool save(const std::string filename);

private:
  uint64_t pixelCount();
  uint64_t blockCount();

  void scaleQuantTables(uint8_t quality);

  void RGBToYCbCr();
  void reorderToBlocks();
  void forwardDCT();
  void forwardDCTBlock(const Block<uint8_t> &input, Block<double> &output);
  void quantize();
  void zigzagReorder();
  void diffEncodeDC();
  void runLengthEncodeAC();
  void runLengthEncodeACBlock(const Block<int8_t> &input, std::vector<RunLengthPair> &output);
  void constructHuffmanEncoders();
  void huffmanEncode();

  static uint8_t huffmanClass(int8_t value);
  static uint8_t huffmanKey(RunLengthPair &pair);

  void encodeAmplitude(int8_t amplitude, std::vector<bool> &output);

  uint64_t m_width;
  uint64_t m_height;

  QuantTable m_quant_table_luma_scaled;
  QuantTable m_quant_table_chroma_scaled;

  std::vector<uint8_t> m_channel_R;
  std::vector<uint8_t> m_channel_G;
  std::vector<uint8_t> m_channel_B;

  std::vector<uint8_t> m_channel_Y_raw;
  std::vector<uint8_t> m_channel_Cb_raw;
  std::vector<uint8_t> m_channel_Cr_raw;

  std::vector<Block<uint8_t>> m_channel_Y_blocky;
  std::vector<Block<uint8_t>> m_channel_Cb_blocky;
  std::vector<Block<uint8_t>> m_channel_Cr_blocky;

  std::vector<Block<double>> m_channel_Y_transformed;
  std::vector<Block<double>> m_channel_Cb_transformed;
  std::vector<Block<double>> m_channel_Cr_transformed;

  std::vector<Block<int8_t>> m_channel_Y_quantized;
  std::vector<Block<int8_t>> m_channel_Cb_quantized;
  std::vector<Block<int8_t>> m_channel_Cr_quantized;

  std::vector<Block<int8_t>> m_channel_Y_zigzag;
  std::vector<Block<int8_t>> m_channel_Cb_zigzag;
  std::vector<Block<int8_t>> m_channel_Cr_zigzag;

  std::vector<int8_t> m_channel_Y_DC;
  std::vector<int8_t> m_channel_Cb_DC;
  std::vector<int8_t> m_channel_Cr_DC;

  std::vector<std::vector<RunLengthPair>> m_channel_Y_AC;
  std::vector<std::vector<RunLengthPair>> m_channel_Cb_AC;
  std::vector<std::vector<RunLengthPair>> m_channel_Cr_AC;

  HuffmanEncoder m_huffman_encoder_luma_DC;
  HuffmanEncoder m_huffman_encoder_luma_AC;
  HuffmanEncoder m_huffman_encoder_chroma_DC;
  HuffmanEncoder m_huffman_encoder_chroma_AC;

  std::vector<bool> m_jpeg2d_data;
};

#endif
