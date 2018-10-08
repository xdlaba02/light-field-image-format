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
  m_channel_Y_run_length_decoded(),
  m_channel_Cb_run_length_decoded(),
  m_channel_Cr_run_length_decoded(),
  m_channel_Y_unzigzaged(),
  m_channel_Cb_unzigzaged(),
  m_channel_Cr_unzigzaged(),
  m_channel_Y_dequantized(),
  m_channel_Cb_dequantized(),
  m_channel_Cr_dequantized(),
  m_channel_Y_detransformed(),
  m_channel_Cb_detransformed(),
  m_channel_Cr_detransformed(),
  m_channel_Y_deblockized(),
  m_channel_Cb_deblockized(),
  m_channel_Cr_deblockized(),
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

  input.read(reinterpret_cast<char *>(m_quant_table_luma.data()), 64);
  input.read(reinterpret_cast<char *>(m_quant_table_chroma.data()), 64);

  m_huffman_decoder_luma_DC.load(input);
  m_huffman_decoder_luma_AC.load(input);
  m_huffman_decoder_chroma_DC.load(input);
  m_huffman_decoder_chroma_AC.load(input);

  char buff = 0;
  while (input.read(&buff, 1)) {
    std::bitset<8> buff_bits(buff);
    for (uint8_t i = 0; i < 8; i++) {
      m_jpeg2d_data.push_back(buff_bits[7 - i]);
    }
    buff = 0;
  }

  m_channel_Y_DC.resize(blockCount());
  m_channel_Cb_DC.resize(blockCount());
  m_channel_Cr_DC.resize(blockCount());

  m_channel_Y_AC.resize(blockCount());
  m_channel_Cb_AC.resize(blockCount());
  m_channel_Cr_AC.resize(blockCount());

  m_channel_Y_run_length_decoded.resize(blockCount());
  m_channel_Cb_run_length_decoded.resize(blockCount());
  m_channel_Cr_run_length_decoded.resize(blockCount());

  m_channel_Y_unzigzaged.resize(blockCount());
  m_channel_Cb_unzigzaged.resize(blockCount());
  m_channel_Cr_unzigzaged.resize(blockCount());

  m_channel_Y_dequantized.resize(blockCount());
  m_channel_Cb_dequantized.resize(blockCount());
  m_channel_Cr_dequantized.resize(blockCount());

  m_channel_Y_detransformed.resize(blockCount());
  m_channel_Cb_detransformed.resize(blockCount());
  m_channel_Cr_detransformed.resize(blockCount());

  m_channel_Y_deblockized.resize(pixelCount());
  m_channel_Cb_deblockized.resize(pixelCount());
  m_channel_Cr_deblockized.resize(pixelCount());

  m_channel_R.resize(pixelCount());
  m_channel_G.resize(pixelCount());
  m_channel_B.resize(pixelCount());

  return true;
}

void JPEG2DDecoder::run(uint64_t &width, uint64_t &height, std::vector<uint8_t> &data) {
  huffmanDecode();
  runLengthDecodeAC();
  diffDecodeDC();
  dezigzag();
  dequantize();
  backwardDCT();
  deblockize();
  YCbCrToRGB();

  width = m_width;
  height = m_height;

  for (uint64_t i = 0; i < pixelCount(); i++) {
    data.push_back(m_channel_R[i]);
    data.push_back(m_channel_G[i]);
    data.push_back(m_channel_B[i]);
  }
}

uint64_t JPEG2DDecoder::blockCount() {
  return ceil(m_width / 8.0) * ceil(m_height / 8.0);
}

uint64_t JPEG2DDecoder::pixelCount() {
  return m_width * m_height;
}

void JPEG2DDecoder::huffmanDecode() {
  std::vector<bool>::iterator it = m_jpeg2d_data.begin();

  RunLengthPair pair;

  for (uint64_t i = 0; i < blockCount(); i++) {
    m_channel_Y_DC[i] = decodeValue(m_huffman_decoder_luma_DC, it).amplitude;
    do {
      pair = decodeValue(m_huffman_decoder_luma_AC, it);
      m_channel_Y_AC[i].push_back(pair);
    } while ((pair.zeroes != 0) || (pair.amplitude != 0));

    m_channel_Cb_DC[i] = decodeValue(m_huffman_decoder_chroma_DC, it).amplitude;
    do {
      pair = decodeValue(m_huffman_decoder_chroma_AC, it);
      m_channel_Cb_AC[i].push_back(pair);
    } while ((pair.zeroes != 0) || (pair.amplitude != 0));

    m_channel_Cr_DC[i] = decodeValue(m_huffman_decoder_chroma_DC, it).amplitude;
    do {
      pair = decodeValue(m_huffman_decoder_chroma_AC, it);
      m_channel_Cr_AC[i].push_back(pair);
    } while ((pair.zeroes != 0) || (pair.amplitude != 0));
  }
}

void JPEG2DDecoder::runLengthDecodeAC() {
  for (uint64_t i = 0; i < blockCount(); i++) {
    runLengthDecodeACBlock(m_channel_Y_AC[i], m_channel_Y_run_length_decoded[i]);
    runLengthDecodeACBlock(m_channel_Cb_AC[i], m_channel_Cb_run_length_decoded[i]);
    runLengthDecodeACBlock(m_channel_Cr_AC[i], m_channel_Cr_run_length_decoded[i]);
  }
}

void JPEG2DDecoder::runLengthDecodeACBlock(const std::vector<RunLengthPair> &input, Block<int8_t> &output) {
  uint8_t index = 1;
  for (auto &pair: input) {
    if ((pair.zeroes == 0) && (pair.amplitude == 0)) {
      break;
    }

    index += pair.zeroes;
    output[index] = pair.amplitude;
    index++;
  }
}

void JPEG2DDecoder::diffDecodeDC() {
  int8_t prev_Y_DC = 0;
  int8_t prev_Cb_DC = 0;
  int8_t prev_Cr_DC = 0;
  for (uint64_t i = 0; i < blockCount(); i++) {
    m_channel_Y_run_length_decoded[i][0] = m_channel_Y_DC[i] + prev_Y_DC;
    m_channel_Cb_run_length_decoded[i][0] = m_channel_Cb_DC[i] + prev_Cb_DC;
    m_channel_Cr_run_length_decoded[i][0] = m_channel_Cr_DC[i] + prev_Cr_DC;
    prev_Y_DC = m_channel_Y_run_length_decoded[i][0];
    prev_Cb_DC = m_channel_Cb_run_length_decoded[i][0];
    prev_Cr_DC = m_channel_Cr_run_length_decoded[i][0];
  }
}

void JPEG2DDecoder::dezigzag() {
  const uint8_t dezigzag_index_table[] = {
     0, 1, 5, 6,14,15,27,28,
     2, 4, 7,13,16,26,29,42,
     3, 8,12,17,25,30,41,43,
     9,11,18,24,31,40,44,53,
    10,19,23,32,39,45,52,54,
    20,22,33,38,46,51,55,60,
    21,34,37,47,50,56,59,61,
    35,36,48,49,57,58,62,63,
  };

  for (uint64_t block = 0; block < blockCount(); block++) {
    for (uint8_t pixel = 0; pixel < 64; pixel++) {
      m_channel_Y_unzigzaged[block][pixel] = m_channel_Y_run_length_decoded[block][dezigzag_index_table[pixel]];
      m_channel_Cb_unzigzaged[block][pixel] = m_channel_Cb_run_length_decoded[block][dezigzag_index_table[pixel]];
      m_channel_Cr_unzigzaged[block][pixel] = m_channel_Cr_run_length_decoded[block][dezigzag_index_table[pixel]];
    }
  }
}

void JPEG2DDecoder::dequantize() {
  for (uint64_t block = 0; block < blockCount(); block++) {
    for (uint8_t pixel = 0; pixel < 64; pixel++) {
      m_channel_Y_dequantized[block][pixel] = m_channel_Y_unzigzaged[block][pixel] * m_quant_table_luma[pixel];
      m_channel_Cb_dequantized[block][pixel] = m_channel_Cb_unzigzaged[block][pixel] * m_quant_table_chroma[pixel];
      m_channel_Cr_dequantized[block][pixel] = m_channel_Cr_unzigzaged[block][pixel] * m_quant_table_chroma[pixel];
    }
  }
}

void JPEG2DDecoder::backwardDCT() {
  for (uint64_t i = 0; i < blockCount(); i++) {
    backwardDCTBlock(m_channel_Y_dequantized[i], m_channel_Y_detransformed[i]);
    backwardDCTBlock(m_channel_Cb_dequantized[i], m_channel_Cb_detransformed[i]);
    backwardDCTBlock(m_channel_Cr_dequantized[i], m_channel_Cr_detransformed[i]);
  }
}

void JPEG2DDecoder::backwardDCTBlock(const Block<double> &input, Block<uint8_t> &output) {
  auto lambda = [](uint8_t val) { return val == 0 ? 1/sqrt(2) : 1; };

  for (uint8_t y = 0; y < 8; y++) {
    for (uint8_t x = 0; x < 8; x++) {
      double sum = 0;

      for (uint8_t v = 0; v < 8; v++) {
        for (uint8_t u = 0; u < 8; u++) {
          double c1 = ((2 * x + 1) * u * PI) / 16;
          double c2 = ((2 * y + 1) * v * PI) / 16;
          sum += input[v * 8 + u] * lambda(u) * lambda(v) * cos(c1) * cos(c2);
        }
      }

      output[y * 8 + x] = (0.25 * sum) + 128;
    }
  }
}

void JPEG2DDecoder::deblockize() {
  for (uint64_t image_y = 0; image_y < m_height; image_y++) {
    for (uint64_t image_x = 0; image_x < m_width; image_x++) {
      uint64_t block_x = image_x / 8;
      uint64_t block_y = image_y / 8;
      uint64_t pixel_x = image_x % 8;
      uint64_t pixel_y = image_y % 8;

      m_channel_Y_deblockized[image_y * m_width + image_x] = m_channel_Y_detransformed[block_y * ceil(m_width/8.0) + block_x][pixel_y * 8 + pixel_x];
      m_channel_Cb_deblockized[image_y * m_width + image_x] = m_channel_Cb_detransformed[block_y * ceil(m_width/8.0) + block_x][pixel_y * 8 + pixel_x];
      m_channel_Cr_deblockized[image_y * m_width + image_x] = m_channel_Cr_detransformed[block_y * ceil(m_width/8.0) + block_x][pixel_y * 8 + pixel_x];
    }
  }
}

void JPEG2DDecoder::YCbCrToRGB() {
  for (uint64_t i = 0; i < pixelCount(); i++) {
    uint8_t Y  = m_channel_Y_deblockized[i];
    uint8_t Cb = m_channel_Cb_deblockized[i];
    uint8_t Cr = m_channel_Cr_deblockized[i];

    m_channel_R[i] = Y + 1.402                            * (Cr - 128);
    m_channel_G[i] = Y - 0.344136 * (Cb - 128) - 0.714136 * (Cr - 128);
    m_channel_B[i] = Y + 1.772    * (Cb - 128);
  }
}

RunLengthPair JPEG2DDecoder::decodeValue(const HuffmanDecoder &decoder, std::vector<bool>::iterator &it) {
  uint8_t pair = decoder.decodeOne(it);
  int8_t amp = decodeAmplitude(pair & 0x0f, it);
  return RunLengthPair(pair >> 4, amp);
}

int8_t JPEG2DDecoder::decodeAmplitude(const uint8_t len, std::vector<bool>::iterator &it) {
  if (len == 0) {
    return 0;
  }

  uint8_t buffer = 0;
  for (uint8_t i = 0; i < len; i++) {
    buffer <<= 1;
    buffer |= *it;
    it++;
  }

  if (buffer < (1 << (len - 1))) {
    buffer |= 0xff << len;
    buffer = ~buffer;
    buffer = -buffer;
  }

  return buffer;
}
