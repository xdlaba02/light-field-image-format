#include "codec.h"
#include "bitmap.h"

#include <cmath>

MyJPEG *encode(Bitmap *image) {
  int image_pixels = image->width() * image->height();

  uint8_t Y[image->width()][image->height()];
  uint8_t Cb[image->width()][image->height()];
  uint8_t Cr[image->width()][image->height()];

  for (int i = 0; i < image_pixels; i++) {
    Bitmap::Pixel pixel = image->getPixel(i);
    Y[i] =        (0.299    * pixel.red + 0.587    * pixel.green + 0.114    * pixel.blue) * 255;
    Cb[i] = 128 - (0.168736 * pixel.red + 0.331264 * pixel.green + 0.5      * pixel.blue) * 255;
    Cr[i] = 128 + (0.5      * pixel.red + 0.418688 * pixel.green + 0.081312 * pixel.blue) * 255;
  }



  return nullptr;
}

Bitmap *decode(MyJPEG *data) {
  return nullptr;
}

double alpha(double u) {
  return u == 0 ? 1/sqrt(2) : 1;
}

void fct(int8_t input[8][8], double output[8][8]) {
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
