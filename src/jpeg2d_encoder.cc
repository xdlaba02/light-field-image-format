#include "jpeg2d_encoder.h"

#include <iostream>
#include <bitset>
#include <fstream>

JPEG2DEncoder::JPEG2DEncoder(const uint64_t width, const uint64_t height, const uint8_t rgb_data[], const uint8_t quality):
  m_width(width),
  m_height(height),
  m_quant_table_luma_scaled(),
  m_quant_table_chroma_scaled(),
  m_channel_R(pixelCount()),
  m_channel_G(pixelCount()),
  m_channel_B(pixelCount()),
  m_channel_Y_raw(pixelCount()),
  m_channel_Cb_raw(pixelCount()),
  m_channel_Cr_raw(pixelCount()),
  m_channel_Y_blocky(blockCount()),
  m_channel_Cb_blocky(blockCount()),
  m_channel_Cr_blocky(blockCount()),
  m_channel_Y_transformed(blockCount()),
  m_channel_Cb_transformed(blockCount()),
  m_channel_Cr_transformed(blockCount()),
  m_channel_Y_quantized(blockCount()),
  m_channel_Cb_quantized(blockCount()),
  m_channel_Cr_quantized(blockCount()),
  m_channel_Y_zigzag(blockCount()),
  m_channel_Cb_zigzag(blockCount()),
  m_channel_Cr_zigzag(blockCount()),
  m_channel_Y_DC(blockCount()),
  m_channel_Cb_DC(blockCount()),
  m_channel_Cr_DC(blockCount()),
  m_channel_Y_AC(blockCount()),
  m_channel_Cb_AC(blockCount()),
  m_channel_Cr_AC(blockCount()),
  m_huffman_encoder_luma_DC(),
  m_huffman_encoder_luma_AC(),
  m_huffman_encoder_chroma_DC(),
  m_huffman_encoder_chroma_AC(),
  m_jpeg2d_data() {

  for (uint64_t i = 0; i < pixelCount(); i++) {
    m_channel_R[i] = rgb_data[3 * i + 0];
    m_channel_G[i] = rgb_data[3 * i + 1];
    m_channel_B[i] = rgb_data[3 * i + 2];
  }

  scaleQuantTables(quality);
}

JPEG2DEncoder::~JPEG2DEncoder() {}

void JPEG2DEncoder::run() {
  RGBToYCbCr();
  reorderToBlocks();
  forwardDCT();
  quantize();
  zigzagReorder();
  diffEncodeDC();
  runLengthEncodeAC();
  constructHuffmanEncoders();
  huffmanEncode();
}

bool JPEG2DEncoder::save(const std::string filename) {
    std::ofstream output(filename);
    if (output.fail()) {
      return false;
    }

    output.write("JPEG-2D\n", 8);


    uint64_t swapped_w = toBigEndian(m_width);
    uint64_t swapped_h = toBigEndian(m_height);

    output.write(reinterpret_cast<char *>(&swapped_w), 8);
    output.write(reinterpret_cast<char *>(&swapped_h), 8);

    output.write(reinterpret_cast<char *>(m_quant_table_luma_scaled.data()), 64);
    output.write(reinterpret_cast<char *>(m_quant_table_chroma_scaled.data()), 64);

    m_huffman_encoder_luma_DC.writeTable(output);
    m_huffman_encoder_luma_AC.writeTable(output);
    m_huffman_encoder_chroma_DC.writeTable(output);
    m_huffman_encoder_chroma_AC.writeTable(output);

    uint8_t i(0);
    uint8_t acc(0);
    for (auto &&bit: m_jpeg2d_data) {
      if (i > 7) {
        output.write(reinterpret_cast<char *>(&acc), sizeof(acc));
        i = 0;
        acc = 0;
      }

      acc |= bit << 7 - i;
      i++;
    }

    if (i > 0) {
      output.write(reinterpret_cast<char *>(&acc), sizeof(acc));
    }

    return true;
}

uint64_t JPEG2DEncoder::pixelCount() {
  return m_width * m_height;
}

uint64_t JPEG2DEncoder::blockCount() {
  return ceil(m_width / 8.0) * ceil(m_height / 8.0);
}

void JPEG2DEncoder::scaleQuantTables(uint8_t quality) {
  //TODO luma chroma ruzne tabulky
  const QuantTable universal_quant_table = {
    16,11,10,16, 24, 40, 51, 61,
    12,12,14,19, 26, 58, 60, 55,
    14,13,16,24, 40, 57, 69, 56,
    14,17,22,29, 51, 87, 80, 62,
    18,22,37,56, 68,109,103, 77,
    24,35,55,64, 81,104,113, 92,
    49,64,78,87,103,121,120,101,
    72,92,95,98,112,100,103, 99
  };

  double scaled_coef;
  if (quality < 50) {
    scaled_coef = (5000.0 / quality) / 100;
  }
  else {
    scaled_coef = (200.0 - 2 * quality) / 100;
  }

  for (uint8_t i = 0; i < 64; i++) {
    uint8_t quant_value = universal_quant_table[i] * scaled_coef;

    if (quant_value < 1) {
      quant_value = 1;
    }

    m_quant_table_luma_scaled[i] = quant_value;
    m_quant_table_chroma_scaled[i] = quant_value;
  }
}

void JPEG2DEncoder::RGBToYCbCr() {
  for (uint64_t i = 0; i < pixelCount(); i++) {
    uint8_t R = m_channel_R[i];
    uint8_t G = m_channel_G[i];
    uint8_t B = m_channel_B[i];

    m_channel_Y_raw[i]  =          0.299 * R +    0.587 * G +    0.114 * B;
    m_channel_Cb_raw[i] = 128 - 0.168736 * R - 0.331264 * G +      0.5 * B;
    m_channel_Cr_raw[i] = 128 +      0.5 * R - 0.418688 * G - 0.081312 * B;
  }
}

void JPEG2DEncoder::reorderToBlocks() {
  for (uint64_t block_y = 0; block_y < ceil(m_height/8.0); block_y++) {
    for (uint64_t block_x = 0; block_x < ceil(m_width/8.0); block_x++) {
      uint64_t block_index = block_y * ceil(m_width/8.0) + block_x;

      for (uint8_t pixel_y = 0; pixel_y < 8; pixel_y++) {
        for (uint8_t pixel_x = 0; pixel_x < 8; pixel_x++) {
          uint64_t image_x = (8 * block_x) + pixel_x;
          uint64_t image_y = (8 * block_y) + pixel_y;

          if (image_x > m_width - 1) {
            image_x = m_width - 1;
          }

          if (image_y > m_height - 1) {
            image_y = m_height - 1;
          }

          uint64_t input_pixel_index = image_y * m_width + image_x;
          uint64_t output_pixel_index = (8 * pixel_y) + pixel_x;

          m_channel_Y_blocky[block_index][output_pixel_index] = m_channel_Y_raw[input_pixel_index];
          m_channel_Cb_blocky[block_index][output_pixel_index] = m_channel_Cb_raw[input_pixel_index];
          m_channel_Cr_blocky[block_index][output_pixel_index] = m_channel_Cr_raw[input_pixel_index];
        }
      }
    }
  }
}

void JPEG2DEncoder::forwardDCT() {
  for (uint64_t i = 0; i < blockCount(); i++) {
    forwardDCTBlock(m_channel_Y_blocky[i], m_channel_Y_transformed[i]);
    forwardDCTBlock(m_channel_Cb_blocky[i], m_channel_Cb_transformed[i]);
    forwardDCTBlock(m_channel_Cr_blocky[i], m_channel_Cr_transformed[i]);
  }
}

void JPEG2DEncoder::forwardDCTBlock(const Block<uint8_t> &input, Block<double> &output) {
  for (uint8_t k = 0; k < 8; k++) {
    for (uint8_t l = 0; l < 8; l++) {
      double sum = 0;

      for (uint8_t m = 0; m < 8; m++) {
        for (uint8_t n = 0; n < 8; n++) {
          double c1 = ((2 * m + 1) * k * PI) / 16;
          double c2 = ((2 * n + 1) * l * PI) / 16;
          sum += (input[m * 8 + n] - 128) * cos(c1) * cos(c2);
        }
      }

      auto lambda = [](uint8_t val) { return val == 0 ? 1/sqrt(2) : 1; };
      output[k * 8 + l] = 0.25 * lambda(k) * lambda(l) * sum;
    }
  }
}

void JPEG2DEncoder::quantize() {
  for (uint64_t block = 0; block < blockCount(); block++) {
    for (uint8_t pixel = 0; pixel < 64; pixel++) {
      m_channel_Y_quantized[block][pixel] = round(m_channel_Y_transformed[block][pixel] / m_quant_table_luma_scaled[pixel]);
      m_channel_Cb_quantized[block][pixel] = round(m_channel_Cb_transformed[block][pixel] / m_quant_table_chroma_scaled[pixel]);
      m_channel_Cr_quantized[block][pixel] = round(m_channel_Cr_transformed[block][pixel] / m_quant_table_chroma_scaled[pixel]);
    }
  }
}

void JPEG2DEncoder::zigzagReorder() {
  const uint8_t zigzag_index_table[] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
  };

  for (uint64_t block = 0; block < blockCount(); block++) {
    for (uint8_t pixel = 0; pixel < 64; pixel++) {
      m_channel_Y_zigzag[block][pixel] = m_channel_Y_quantized[block][zigzag_index_table[pixel]];
      m_channel_Cb_zigzag[block][pixel] = m_channel_Cb_quantized[block][zigzag_index_table[pixel]];
      m_channel_Cr_zigzag[block][pixel] = m_channel_Cr_quantized[block][zigzag_index_table[pixel]];
    }
  }
}

void JPEG2DEncoder::diffEncodeDC() {
  int8_t prev_Y_DC = 0;
  int8_t prev_Cb_DC = 0;
  int8_t prev_Cr_DC = 0;
  for (uint64_t i = 0; i < blockCount(); i++) {
    m_channel_Y_DC[i] = m_channel_Y_zigzag[i][0] - prev_Y_DC;
    m_channel_Cb_DC[i] = m_channel_Cb_zigzag[i][0] - prev_Cb_DC;
    m_channel_Cr_DC[i] = m_channel_Cr_zigzag[i][0] - prev_Cr_DC;
  }
}

void JPEG2DEncoder::runLengthEncodeAC() {
  for (uint64_t i = 0; i < blockCount(); i++) {
    runLengthEncodeACBlock(m_channel_Y_zigzag[i], m_channel_Y_AC[i]);
    runLengthEncodeACBlock(m_channel_Cb_zigzag[i], m_channel_Cb_AC[i]);
    runLengthEncodeACBlock(m_channel_Cr_zigzag[i], m_channel_Cr_AC[i]);
  }
}

void JPEG2DEncoder::runLengthEncodeACBlock(const Block<int8_t> &input, std::vector<RunLengthPair> &output) {
  uint8_t zeroes = 0;
  for (uint8_t i = 1; i < 64; i++) {
    if (input[i] == 0) {
      zeroes++;
    }
    else {
      while (zeroes >= 16) {
        output.push_back(RunLengthPair(15, 0));
        zeroes -= 16;
      }
      output.push_back(RunLengthPair(zeroes, input[i]));
      zeroes = 0;
    }
  }
  output.push_back(RunLengthPair(0, 0));
}

void JPEG2DEncoder::constructHuffmanEncoders() {
  for (uint64_t i = 0; i < blockCount(); i++) {
    m_huffman_encoder_luma_DC.incrementKey(huffmanClass(m_channel_Y_DC[i]));
    m_huffman_encoder_chroma_DC.incrementKey(huffmanClass(m_channel_Cb_DC[i]));
    m_huffman_encoder_chroma_DC.incrementKey(huffmanClass(m_channel_Cr_DC[i]));

    for (auto &pair: m_channel_Y_AC[i]) {
      m_huffman_encoder_luma_AC.incrementKey(huffmanKey(pair));
    }

    for (auto &pair: m_channel_Cb_AC[i]) {
      m_huffman_encoder_chroma_AC.incrementKey(huffmanKey(pair));
    }

    for (auto &pair: m_channel_Cr_AC[i]) {
      m_huffman_encoder_chroma_AC.incrementKey(huffmanKey(pair));
    }
  }

  m_huffman_encoder_luma_AC.constructTable();
  m_huffman_encoder_luma_DC.constructTable();

  m_huffman_encoder_chroma_DC.constructTable();
  m_huffman_encoder_chroma_AC.constructTable();
}

void JPEG2DEncoder::huffmanEncode() {
  for (uint64_t i = 0; i < blockCount(); i++) {
    m_huffman_encoder_luma_DC.encode(huffmanClass(m_channel_Y_DC[i]), m_jpeg2d_data);
    encodeAmplitude(m_channel_Y_DC[i], m_jpeg2d_data);

    for (auto &pair: m_channel_Y_AC[i]) {
      m_huffman_encoder_luma_AC.encode(huffmanKey(pair), m_jpeg2d_data);
      encodeAmplitude(pair.amplitude, m_jpeg2d_data);
    }

    m_huffman_encoder_chroma_DC.encode(huffmanClass(m_channel_Cb_DC[i]), m_jpeg2d_data);
    encodeAmplitude(m_channel_Cb_DC[i], m_jpeg2d_data);

    for (auto &pair: m_channel_Cb_AC[i]) {
      m_huffman_encoder_chroma_AC.encode(huffmanKey(pair), m_jpeg2d_data);
      encodeAmplitude(pair.amplitude, m_jpeg2d_data);
    }

    m_huffman_encoder_chroma_DC.encode(huffmanClass(m_channel_Cr_DC[i]), m_jpeg2d_data);
    encodeAmplitude(m_channel_Cr_DC[i], m_jpeg2d_data);

    for (auto &pair: m_channel_Cr_AC[i]) {
      m_huffman_encoder_chroma_AC.encode(huffmanKey(pair), m_jpeg2d_data);
      encodeAmplitude(pair.amplitude, m_jpeg2d_data);
    }
  }
}

uint8_t JPEG2DEncoder::huffmanClass(int8_t value) {
  if (value < 0) {
    value = -value;
  }

  int huff_class = 0;
  while (value > 0) {
    value = value >> 1;
    huff_class++;
  }

  return huff_class;
}

uint8_t JPEG2DEncoder::huffmanKey(RunLengthPair &pair) {
  return pair.zeroes << 4 | huffmanClass(pair.amplitude);
}

void JPEG2DEncoder::encodeAmplitude(int8_t amplitude, std::vector<bool> &output) {
  uint8_t huff_class = huffmanClass(amplitude);
  if (amplitude < 0) {
    amplitude = -amplitude;
    amplitude = ~amplitude;
  }

  std::bitset<8> bits(amplitude);
  for (uint8_t i = 8 - huff_class; i < 8; i++) {
    output.push_back(bits[i]);
  }
}
