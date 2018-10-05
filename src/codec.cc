#include "codec.h"

#include <cmath>

JPEG2DEncoder::JPEG2DEncoder(const uint64_t width, const uint64_t height, const uint8_t rgb_data[], const uint8_t quality):
  m_width(width),
  m_height(height),
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
  m_quant_table_luma(0),
  m_quant_table_chroma(0),
  m_huffman_table_luma_DC(),
  m_huffman_table_luma_AC(),
  m_huffman_table_chroma_DC(),
  m_huffman_table_chroma_AC(),
  m_encoded_image() {

  for (uint64_t i = 0; i < pixelCount(); i++) {
    m_channel_R[i] = rgb_data[3 * i + 0];
    m_channel_G[i] = rgb_data[3 * i + 1];
    m_channel_B[i] = rgb_data[3 * i + 2];
  }

  scaleQuantTables(quality);
}

void JPEG2DEncoder::run() {
  RGBToYCbCr();
  reorderToBlocks();
  forwardDCT();
  quantize();
  zigzagReorder();
  entropyEncode();
}

bool JPEG2DEncoder::save(const std::string filename) {

}

uint64_t JPEG2DEncoder::pixelCount() {
  return m_width * m_height;
}

uint64_t JPEG2DEncoder::blockCount() {
  return ceil(m_width / 8.0) * ceil(m_height / 8.0);
}

void JPEG2DEncoder::scaleQuantTables(uint8_t quality) {
  //TODO luma chroma ruzne tabulky
  const QuantTable universal_quan_table = {
    16,11,10,16, 24, 40, 51, 61,
    12,12,14,19, 26, 58, 60, 55,
    14,13,16,24, 40, 57, 69, 56,
    14,17,22,29, 51, 87, 80, 62,
    18,22,37,56, 68,109,103, 77,
    24,35,55,64, 81,104,113, 92,
    49,64,78,87,103,121,120,101,
    72,92,95,98,112,100,103, 99
  };

  uint8_t scaled_quality;
  if (quality < 50) {
    scaled_quality = 5000 / quality;
  }
  else {
    scaled_quality = 200 - 2 * quality;
  }

  for (uint8_t i = 0; i < 64; i++) {
    uint8_t quant_value = (universal_quan_table[i] * scaled_quality) / 100;

    if (quant_value < 1) {
      quant_value = 1;
    }

    m_quant_table_luma[i] = quant_value;
    m_quant_table_chroma[i] = quant_value;
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
      uint64_t block_index = block_y * ceil(m_width/8.0) * block_x;

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

          uint64_t input_pixel_index = image_y * m_width * image_x;
          uint64_t output_pixel_index = pixel_y * 8 * pixel_x;

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
  static const double PI = 4 * atan(1);

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
  for (uint64_t i = 0; i < blockCount(); i++) {
    for (uint8_t i = 0; i < 64; i++) {
      m_channel_Y_quantized[i] = round(m_channel_Y_transformed[i]/m_quant_table_luma_scaled[i]);
      m_channel_Cb_quantized[i] = round(m_channel_Cb_transformed[i]/m_quant_table_chroma_scaled[i]);
      m_channel_Cr_quantized[i] = round(m_channel_Cr_transformed[i]/m_quant_table_chroma_scaled[i]);
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

  for (uint64_t i = 0; i < blockCount(); i++) {
    for (uint8_t i = 0; i < 64; i++) {
      m_channel_Y_zigzag[i] = m_channel_Y_quantized[zigzag_index_table[i]];
      m_channel_Cb_zigzag[i] = m_channel_Cb_quantized[zigzag_index_table[i]];
      m_channel_Cr_zigzag[i] = m_channel_Cr_quantized[zigzag_index_table[i]];
    }
  }
}

void JPEG2DEncoder::entropyEncode() {
  int8_t prev_Y_DC = 0;
  int8_t prev_Cb_DC = 0;
  int8_t prev_Cr_DC = 0;
  for (uint64_t i = 0; i < blockCount(); i++) {
    m_channel_Y_DC[i] = m_channel_Y_zigzag[i][0] - prev_Y_DC;
    m_channel_Cb_DC[i] = m_channel_Cb_zigzag[i][0] - prev_Cb_DC;
    m_channel_Cr_DC[i] = m_channel_Cr_zigzag[i][0] - prev_Cr_DC;
  }
}

void jpegEncode(const BitmapRGB &input, const uint8_t quality, JPEG2D &output) {
  output.width = input.width();
  output.height = input.height();

  BitmapG Y;
  BitmapG Cb;
  BitmapG Cr;

  RGBToYCbCr(input, Y, Cb, Cr);

  std::vector<RLTuple> runlengthEncodedY;
  std::vector<RLTuple> runlengthEncodedCb;
  std::vector<RLTuple> runlengthEncodedCr;

  scaleQuantizationTable(universal_quantization_table, quality, output.quant_table_luma);
  scaleQuantizationTable(universal_quantization_table, quality, output.quant_table_chroma);

  encodeChannel(Y, output.quant_table_luma, runlengthEncodedY);
  encodeChannel(Cb, output.quant_table_chroma, runlengthEncodedCb);
  encodeChannel(Cr, output.quant_table_chroma, runlengthEncodedCr);

  fillHuffmanTables(runlengthEncodedY, output.huffman_luma_DC, output.huffman_luma_AC);
  fillHuffmanTables(runlengthEncodedCb, output.huffman_chroma_DC, output.huffman_chroma_AC);
  fillHuffmanTables(runlengthEncodedCr, output.huffman_chroma_DC, output.huffman_chroma_AC);

  output.huffman_luma_DC.constructTable();
  output.huffman_luma_AC.constructTable();

  output.huffman_chroma_DC.constructTable();
  output.huffman_chroma_AC.constructTable();

  std::vector<RLTuple>::iterator it_Y = runlengthEncodedY.begin();
  std::vector<RLTuple>::iterator it_Cb = runlengthEncodedCb.begin();
  std::vector<RLTuple>::iterator it_Cr = runlengthEncodedCr.begin();

  Bitstream &bitstream = output.data;
  BitField codeword;
  BitField encodedValue;

  while (it_Y != runlengthEncodedY.end()) {
    codeword = output.huffman_luma_DC.getCodeword(*it_Y);
    bitstream.append(codeword.value, codeword.length);

    encodedValue = HuffmanTable::getEncodedValue(*it_Y);
    bitstream.append(encodedValue.value, encodedValue.length);

    do {
      it_Y++;
      codeword = output.huffman_luma_AC.getCodeword(*it_Y);
      bitstream.append(codeword.value, codeword.length);

      encodedValue = HuffmanTable::getEncodedValue(*it_Y);
      bitstream.append(encodedValue.value, encodedValue.length);
    } while ((*it_Y).zeroes != 0 && (*it_Y).amplitude != 0);

    codeword = output.huffman_chroma_DC.getCodeword(*it_Cb);
    bitstream.append(codeword.value, codeword.length);

    encodedValue = HuffmanTable::getEncodedValue(*it_Cb);
    bitstream.append(encodedValue.value, encodedValue.length);

    do {
      it_Cb++;
      codeword = output.huffman_chroma_AC.getCodeword(*it_Cb);
      bitstream.append(codeword.value, codeword.length);

      encodedValue = HuffmanTable::getEncodedValue(*it_Cb);
      bitstream.append(encodedValue.value, encodedValue.length);
    } while ((*it_Cb).zeroes != 0 && (*it_Cb).amplitude != 0);

    codeword = output.huffman_chroma_DC.getCodeword(*it_Cr);
    bitstream.append(codeword.value, codeword.length);

    encodedValue = HuffmanTable::getEncodedValue(*it_Cr);
    bitstream.append(encodedValue.value, encodedValue.length);

    do {
      it_Cr++;
      codeword = output.huffman_chroma_AC.getCodeword(*it_Cr);
      bitstream.append(codeword.value, codeword.length);

      encodedValue = HuffmanTable::getEncodedValue(*it_Cr);
      bitstream.append(encodedValue.value, encodedValue.length);
    } while ((*it_Cr).zeroes != 0 && (*it_Cr).amplitude != 0);
  }

  bitstream.flush();
}

void fillHuffmanTables(const std::vector<RLTuple> &input, HuffmanTable &DC, HuffmanTable &AC) {
  ACDCState state = STATE_DC;

  for (auto &tuple: input) {
    switch (state) {
      case STATE_DC:
      DC.addKey(tuple);
      state = STATE_AC;
      break;

      case STATE_AC:
      AC.addKey(tuple);
      if (tuple.amplitude == 0 && tuple.zeroes == 0) {
        state = STATE_DC;
      }
      break;
    }
  }
}

void encodeChannel(const BitmapG &input, const Block<uint8_t> &quant_table, std::vector<RLTuple> &output) {
  BitmapG blocky;
  splitToBlocks(input, blocky);

  int8_t prev = 0;
  for (uint64_t i = 0; i < blocky.sizeInPixels(); i += 64) {
    Block<uint8_t> &block = reinterpret_cast<Block<uint8_t> &>(blocky.data()[i]);
    Block<double> freq_block;
    Block<int8_t> quant_block;
    Block<int8_t> zigzag_data;

    fct(block, freq_block);
    quantize(freq_block, quant_table, quant_block);
    zigzag(quant_block, zigzag_data);

    int8_t DC_coef = zigzag_data[0];
    zigzag_data[0] -= prev;
    prev = DC_coef;

    runlengthEncode(zigzag_data, output);
  }
}

void runlengthEncode(const Block<int8_t> &input, std::vector<RLTuple> &output) {
  uint8_t zeroes = 0;
  output.push_back(RLTuple(0, input[0]));
  for (uint8_t i = 1; i < 64; i++) {
    if (input[i] == 0) {
      zeroes++;
    }
    else {
      while (zeroes >= 16) {
        output.push_back(RLTuple(15, 0));
        zeroes -= 16;
      }
      output.push_back(RLTuple(zeroes, input[i]));
      zeroes = 0;
    }
  }
  output.push_back(RLTuple(0, 0));
}
