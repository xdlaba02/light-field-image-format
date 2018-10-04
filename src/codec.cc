#include "codec.h"
#include "bitmap.h"
#include "huffman.h"
#include "jpeg2d.h"

#include <fstream>

#include <cmath>

void saveJPEG(std::string filename, JPEG2D &jpeg) {
  std::ofstream output(filename);
  if (output.fail()) {
    return;
  }

  std::cout << "width: " << jpeg.width << std::endl;
  std::cout << "height: " << jpeg.height << std::endl;
  std::cout << std::endl;
  std::cout << "Luma kvantizacni tabulka: " << std::endl;
  for (uint8_t i = 0; i < 8; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      std::cout << static_cast<unsigned>(jpeg.quant_table_luma[i * 8 + j]) << ", ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  std::cout << "Chroma kvantizacni tabulka: " << std::endl;
  for (uint8_t i = 0; i < 8; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      std::cout << static_cast<unsigned>(jpeg.quant_table_chroma[i * 8 + j]) << ", ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  std::cout << "Luma DC huffmanova tabulka: " << std::endl;
  jpeg.huffman_luma_DC.printTable();
  std::cout << std::endl;
  std::cout << "Luma AC huffmanova tabulka: " << std::endl;
  jpeg.huffman_luma_AC.printTable();
  std::cout << std::endl;
  std::cout << "Chroma DC huffmanova tabulka: " << std::endl;
  jpeg.huffman_chroma_DC.printTable();
  std::cout << std::endl;
  std::cout << "Chroma AC huffmanova tabulka: " << std::endl;
  jpeg.huffman_chroma_AC.printTable();

  //vytiskni nejaku magic neco
  //sira a vyska vysledneho obrazku
  //kvantizacni tabulky
  //huffmanovy tabulky
  //data
}

void loadJPEG(std::string filename, JPEG2D &jpeg) {

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

void RGBToYCbCr(const BitmapRGB &input, BitmapG &Y, BitmapG &Cb, BitmapG &Cr) {
  Y.init(input.width(), input.height());
  Cb.init(input.width(), input.height());
  Cr.init(input.width(), input.height());

  for (uint64_t i = 0; i < input.sizeInPixels(); i++) {
    uint8_t R = input.data()[3 * i + 0];
    uint8_t G = input.data()[3 * i + 1];
    uint8_t B = input.data()[3 * i + 2];

    Y.data()[i]  = RGBtoY(R, G, B);
    Cb.data()[i] = RGBtoCb(R, G, B);
    Cr.data()[i] = RGBtoCr(R, G, B);
  }
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

void scaleQuantizationTable(const Block<uint8_t> &input, const uint8_t quality, Block<uint8_t> &output) {
  uint8_t scaled_quality;
  if (quality < 50) {
    scaled_quality = 5000 / quality;
  }
  else {
    scaled_quality = 200 - 2 * quality;
  }

  for (uint8_t i = 0; i < 64; i++) {
    uint8_t quant_value = (input[i] * scaled_quality) / 100;
    if (quant_value < 1) {
      quant_value = 1;
    }
    output[i] = quant_value;
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

void splitToBlocks(const BitmapG &input, BitmapG &output) {
  uint64_t blocks_width = ceil(input.width() / 8.0);
  uint64_t blocks_height = ceil(input.height() / 8.0);

  output.init(blocks_width * 8, blocks_height * 8);

  for (uint64_t j = 0; j < blocks_height; j++) {
    for (uint64_t i = 0; i < blocks_width; i++) {
      for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
          uint64_t input_x = (8 * i) + x;
          uint64_t input_y = (8 * j) + y;

          if (input_x > input.width() - 1) {
            input_x = input.width() - 1;
          }

          if (input_y > input.height() - 1) {
            input_y = input.height() - 1;
          }

          uint64_t input_index = input_y * input.width() + input_x;
          uint64_t output_index = (j * blocks_width + i) * 64 + (y * 8 + x);

          output.data()[output_index]  = input.data()[input_index];
        }
      }
    }
  }
}

void fct(const Block<uint8_t> &input, Block<double> &output) {
  static const double PI = 4 * atan(double(1));

  for (int k = 0; k < 8; k++) {
    for (int l = 0; l < 8; l++) {
      double sum = 0;

      for (int m = 0; m < 8; m++) {
        for (int n = 0; n < 8; n++) {
          double c1 = ((2*m + 1) * k * PI) / 16;
          double c2 = ((2*n + 1) * l * PI) / 16;
          sum += (input[m * 8 + n] - 128) * cos(c1) * cos(c2);
        }
      }

      output[k * 8 + l] = 0.25 * (k == 0 ? 1/sqrt(2) : 1) * (l == 0 ? 1/sqrt(2) : 1) * sum;
    }
  }
}

void quantize(const Block<double> &input, const Block<uint8_t> &quant_table, Block<int8_t> &output) {
  for (uint8_t i = 0; i < 64; i++) {
    output[i] = round(input[i]/quant_table[i]);
  }
}

void zigzag(const Block<int8_t> &input, Block<int8_t> &output) {
  // 0  1  5  6  14 15 27 28
  // 2  4  7  13 16 26 29 42
  // 3  8  12 17 25 30 41 43
  // 9  11 18 24 31 40 44 53
  // 10 19 23 32 39 45 52 54
  // 20 22 33 38 46 51 55 60
  // 21 34 37 47 50 56 59 61
  // 35 36 48 49 57 58 62 63

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

  for (uint8_t i = 0; i < 64; i++) {
    output[i] = input[zigzag_index_table[i]];
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

uint8_t RGBtoY(uint8_t R, uint8_t G, uint8_t B) {
  return 0.299 * R + 0.587 * G + 0.114 * B;
}

uint8_t RGBtoCb(uint8_t R, uint8_t G, uint8_t B) {
  return 128 - 0.168736 * R - 0.331264 * G + 0.5 * B;
}

uint8_t RGBtoCr(uint8_t R, uint8_t G, uint8_t B) {
  return 128 + 0.5 * R - 0.418688 * G - 0.081312 * B;
}
