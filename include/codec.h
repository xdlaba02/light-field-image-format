#ifndef CODEC_H
#define CODEC_H

#include "huffman.h"

#include <cstdint>
#include <vector>

template<typename T>
using Block = T[64];

using QuantTable = Block<uint8_t>;

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
  void constructHuffmanTables();

  uint64_t m_width;
  uint64_t m_height;

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
  std::vector<Block<int8_t>> m_channel_Cb_qunatized;
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

  QuantTable m_quant_table_luma_scaled;
  QuantTable m_quant_table_chroma_scaled;

  HuffmanTable m_huffman_table_luma_DC;
  HuffmanTable m_huffman_table_luma_AC;
  HuffmanTable m_huffman_table_chroma_AC;
  HuffmanTable m_huffman_table_chroma_DC;

  EncodedImage m_encoded_image;
};

struct RLTuple {
  RLTuple(uint8_t z, int16_t a):zeroes(z), amplitude(a) {}
  uint8_t zeroes;
  int16_t amplitude;
};

enum ACDCState {
  STATE_DC,
  STATE_AC
};

void saveJPEG(std::string filename, JPEG2D &jpeg);
void loadJPEG(std::string filename, JPEG2D &jpeg);

void jpegEncode(const BitmapRGB &input, const uint8_t quality, JPEG2D &output);
void RGBToYCbCr(const BitmapRGB &input, BitmapG &Y, BitmapG &Cb, BitmapG &Cr);
void scaleQuantizationTable(const Block<uint8_t> &input, const uint8_t quality, Block<uint8_t> &output);
void fillHuffmanTables(const std::vector<RLTuple> &input, HuffmanTable &DC, HuffmanTable &AC);

void encodeChannel(const BitmapG &input, const Block<uint8_t> &quant_table, std::vector<RLTuple> &output);
void splitToBlocks(const BitmapG &input, BitmapG &output);

void fct(const Block<uint8_t> &input, Block<double> &output);
void quantize(const Block<double> &input, const Block<uint8_t> &quant_table, Block<int8_t> &output);
void zigzag(const Block<int8_t> &input, Block<int8_t> &output);
void runlengthEncode(const Block<int8_t> &input, std::vector<RLTuple> &output);

uint8_t RGBtoY(uint8_t R, uint8_t G, uint8_t B);
uint8_t RGBtoCb(uint8_t R, uint8_t G, uint8_t B);
uint8_t RGBtoCr(uint8_t R, uint8_t G, uint8_t B);

#endif
