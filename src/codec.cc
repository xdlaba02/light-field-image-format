#include "codec.h"
#include "bitmap.h"

#include <iostream>
#include <cmath>

bool jpegEncode(const BitmapRGB &rgb, const uint8_t quality, ImageJPEG &jpeg) {
  BitmapG Y(rgb.width(), rgb.height());
  BitmapG Cb(rgb.width(), rgb.height());
  BitmapG Cr(rgb.width(), rgb.height());

  RGBToYCbCr(rgb, Y, Cb, Cr);

  uint64_t blocks_x = ceil(rgb.width() / 8);
  uint64_t blocks_y = ceil(rgb.height() / 8);

  for (uint64_t i = 0; i < blocks_x; i++) {
    for (uint64_t j = 0; j < blocks_y; j++) {
      uint8_t input_block[8][8];
      double freq_block[8][8];
      int8_t quant_block[8][8];
      int8_t zigzag_data[64];

      getBlock(i, j, Y, input_block);
      fct(input_block, freq_block);
      quantize(freq_block, quant_block);
      zigzag(quant_block, zigzag_data);

      for (int i = 0; i < 64; i++)
        std::cout << static_cast<int>(zigzag_data[i]) << " ";
      std::cout << "\n";
      //runlength(zigzag, output);
    }
  }

  return true;
}

bool jpegDecode(const ImageJPEG &jpeg, BitmapRGB &rgb) {
  return false;
}

uint8_t bits(int8_t value) {
  uint8_t i = 0;
  while(value) {
    i++;
    value >>= 1;
  }
  return i;
}

void RGBToYCbCr(const BitmapRGB &rgb, BitmapG &Y, BitmapG &Cb, BitmapG &Cr) {
  for (uint64_t i = 0; i < rgb.sizeInPixels(); i++) {
    uint8_t R = rgb.data()[3 * i + 0];
    uint8_t G = rgb.data()[3 * i + 1];
    uint8_t B = rgb.data()[3 * i + 2];

    Y.data()[i]  =   0 + 0.299    * R + 0.587    * G + 0.114    * B;
    Cb.data()[i] = 128 - 0.168736 * R - 0.331264 * G + 0.5      * B;
    Cr.data()[i] = 128 + 0.5      * R - 0.418688 * G - 0.081312 * B;
  }
}

void getBlock(uint64_t x, uint64_t y, BitmapG &input, uint8_t output[8][8]) {
  for (uint8_t i = 0; i < 8; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      uint64_t index_x = (8 * x) + j;
      uint64_t index_y = (8 * y) + i;
      if (index_x > input.width() || index_y > input.height()) {
        output[i][j] = 0;
      }
      else {
        output[i][j] = input.data()[index_y * input.width() + index_x];
      }
    }
  }
}

double alpha(double u) {
  return u == 0 ? 1/sqrt(2) : 1;
}

void fct(uint8_t input[8][8], double output[8][8]) {
  static const double PI = 4 * atan(double(1));

  for (int k = 0; k < 8; k++) {
    for (int l = 0; l < 8; l++) {
      double sum = 0;

      for (int m = 0; m < 8; m++) {
        for (int n = 0; n < 8; n++) {
          double c1 = ((2*m + 1) * k * PI) / 16;
          double c2 = ((2*n + 1) * l * PI) / 16;
          sum += (input[m][n] - 128) * cos(c1) * cos(c2);
        }
      }

      output[k][l] = 0.25 * alpha(k) * alpha(l) * sum;
    }
  }
}

void quantize(double input[8][8], int8_t output[8][8]) {
  uint8_t quantization_matrix[8][8] = {
    {16,11,10,16,24 ,40 ,51 ,61},
    {12,12,14,19,26 ,58 ,60 ,55},
    {14,13,16,24,40 ,57 ,69 ,56},
    {14,17,22,29,51 ,87 ,80 ,62},
    {18,22,37,56,68 ,109,103,77},
    {24,35,55,64,81 ,104,113,92},
    {49,64,78,87,103,121,120,101},
    {72,92,95,98,112,100,103,99}
  };

  for (uint8_t i = 0; i < 8; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      output[i][j] = round(input[i][j]/quantization_matrix[i][j]);
    }
  }
}

int8_t zigzagAt(int8_t input[8][8], uint8_t index) {
 const uint8_t x[] = {0, 1, 0, 0, 1, 2, 3, 2,
                      1, 0, 0, 1, 2, 3, 4, 5,
                      4, 3, 2, 1, 0, 0, 1, 2,
                      3, 4, 5, 6, 7, 6, 5, 4,
                      3, 2, 1, 0, 1, 2, 3, 4,
                      5, 6, 7, 7, 6, 5, 4, 3,
                      2, 3, 4, 5, 6, 7, 7, 6,
                      5, 4, 5, 6, 7, 7, 6, 7};

 const uint8_t y[] = {0, 0, 1, 2, 1, 0, 0, 1,
                      2, 3, 4, 3, 2, 1, 0, 0,
                      1, 2, 3, 4, 5, 6, 5, 4,
                      3, 2, 1, 0, 0, 1, 2, 3,
                      4, 5, 6, 7, 7, 6, 5, 4,
                      3, 2, 1, 2, 3, 4, 5, 6,
                      7, 7, 6, 5, 4, 3, 4, 5,
                      6, 7, 7, 6, 5, 6, 7, 7};

  return input[y[index]][x[index]];
}

void zigzag(int8_t input[8][8], int8_t output[64]) {
  for (uint8_t i = 0; i < 64; i++) {
    output[i] = zigzagAt(input, i);
  }
}

/*void runlength(int8_t input[63], std::vector<RunLengthData> &output) {
  uint8_t zeroes = 0;
  for (uint8_t i = 0; i < 63; i++) {
    if (input[i] == 0) {
      zeroes++;
    }
    else {
      while (zeroes >= 16) {
        output.push_back(RunLengthData(15, 0));
        zeroes -= 16;
      }
      output.push_back(RunLengthData(zeroes, input[i]));
      zeroes = 0;
    }
  }
  output.push_back(RunLengthData(0, 0));
}
*/
