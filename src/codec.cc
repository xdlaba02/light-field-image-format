#include "codec.h"
#include "bitmap.h"

#include <iostream>
#include <cmath>

void jpegEncode(const BitmapRGB &input, const uint8_t quality, ImageJPEG &output) {
  BitmapG Y;
  BitmapG Cb;
  BitmapG Cr;

  RGBToYCbCr(input, Y, Cb, Cr);

  std::vector<RLTuple> runlengthEncodedY;
  std::vector<RLTuple> runlengthEncodedCb;
  std::vector<RLTuple> runlengthEncodedCr;

  encodeChannel(Y, quality, runlengthEncodedY);
  encodeChannel(Y, quality, runlengthEncodedCb);
  encodeChannel(Y, quality, runlengthEncodedCr);


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

void encodeChannel(const BitmapG &input, const uint8_t quality, std::vector<RLTuple> &output) {
  BitmapG blocky;
  splitToBlocks(input, blocky);

  for (uint64_t i = 0; i < blocky.sizeInPixels(); i += 64) {
    uint8_t *data = &blocky.data()[i];
    Block<uint8_t> *block = reinterpret_cast<Block<uint8_t> *>(data);
    encodeBlock(*block, quality, output);
  }
}

void encodeBlock(const Block<uint8_t> &input, const uint8_t quality, std::vector<RLTuple> &output) {
  Block<double> freq_block;
  Block<int8_t> quant_block;
  Block<int8_t> zigzag_block;

  fct(input, freq_block);
  quantize(freq_block, quality, quant_block);
  zigzag(quant_block, zigzag_block);
  runlengthEncode(zigzag_block, output);
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

void quantize(const Block<double> &input, const uint8_t quality, Block<int8_t> &output) {
  const Block<uint8_t> quantization_table = {
    16,11,10,16,24 ,40 ,51 ,61,
    12,12,14,19,26 ,58 ,60 ,55,
    14,13,16,24,40 ,57 ,69 ,56,
    14,17,22,29,51 ,87 ,80 ,62,
    18,22,37,56,68 ,109,103,77,
    24,35,55,64,81 ,104,113,92,
    49,64,78,87,103,121,120,101,
    72,92,95,98,112,100,103,99
  };

  uint8_t scaled_quality;

  if ( quality < 50 ) {
    scaled_quality = 5000 / quality;
  }
  else {
    scaled_quality = 200 - 2 * quality;
  }

  for (uint8_t i = 0; i < 64; i++) {
    uint8_t quant_value = (quantization_table[i] * scaled_quality) / 100;
    if (quant_value < 1) {
      quant_value = 1;
    }
    output[i] = round(input[i]/quant_value);
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
  for (uint8_t i = 0; i < 64; i++) {
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
