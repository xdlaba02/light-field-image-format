#include "codec.h"
#include <cmath>

uint8_t *encodeRGB(uint64_t width, uint64_t height, uint8_t *RGBdata) {
  uint64_t image_pixels = width * height;

  uint8_t *Y  = new uint8_t[image_pixels];
  uint8_t *Cb = new uint8_t[image_pixels];
  uint8_t *Cr = new uint8_t[image_pixels];

  for (uint64_t i = 0; i < image_pixels; i++) {
    RGBtoYCbCr(RGBdata[3 * i + 0],
               RGBdata[3 * i + 1],
               RGBdata[3 * i + 2],
               Y[i],
               Cb[i],
               Cr[i]);
  }

  return nullptr;
}

void RGBtoYCbCr(uint8_t R,
                uint8_t G,
                uint8_t B,
                uint8_t &Y,
                uint8_t &Cb,
                uint8_t &Cr) {
  Y  =   0 + 0.299    * R + 0.587    * G + 0.114    * B;
  Cb = 128 - 0.168736 * R - 0.331264 * G + 0.5      * B;
  Cr = 128 + 0.5      * R - 0.418688 * G - 0.081312 * B;
}

void getBlock(uint64_t x,
              uint64_t y,
              uint64_t width,
              uint64_t height,
              uint8_t *input,
              uint8_t output[8][8]) {
  for (uint8_t i = 0; i < 8; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      uint64_t index_x = (8 * x) + i;
      uint64_t index_y = (8 * y) + j;
      if (index_x > width || index_y > height) {
        output[i][j] = 0;
      }
      else {
        output[i][j] = input[index_y * width + index_x];
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
          sum += input[m][n] * cos(c1) * cos(c2);
        }
      }

      output[k][l] = 0.25 * alpha(k) * alpha(l) * sum;
    }
  }
}
