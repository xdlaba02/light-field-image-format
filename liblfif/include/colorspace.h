/******************************************************************************\
* SOUBOR: colorspace.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef COLORSPACE_H
#define COLORSPACE_H

#include <functional>

inline YCbCrDataUnit RGBtoY(RGBDataUnit R, RGBDataUnit G, RGBDataUnit B) {
  return (0.299 * R) + (0.587 * G) + (0.114 * B);
}

inline YCbCrDataUnit RGBtoCb(RGBDataUnit R, RGBDataUnit G, RGBDataUnit B) {
  return 128 - (0.168736 * R) - (0.331264 * G) + (0.5 * B);
}

inline YCbCrDataUnit RGBtoCr(RGBDataUnit R, RGBDataUnit G, RGBDataUnit B) {
  return 128 + (0.5 * R) - (0.418688 * G) - (0.081312 * B);
}

template<size_t D>
inline YCbCrDataBlock<D> convertRGBDataBlock(const RGBDataBlock<D> &input, function<YCbCrDataUnit(RGBDataUnit, RGBDataUnit, RGBDataUnit)> &&f) {
  YCbCrDataBlock<D> output {};

  for (size_t i = 0; i < constpow(8, D); i++) {
    RGBDataUnit R = input[i].r;
    RGBDataUnit G = input[i].g;
    RGBDataUnit B = input[i].b;

    output[i] = f(R, G, B);
  }

  return output;
}

template<size_t D>
RGBDataBlock<D> YCbCrDataBlockToRGBDataBlock(const YCbCrDataBlock<D> &input_Y, const YCbCrDataBlock<D> &input_Cb, const YCbCrDataBlock<D> &input_Cr) {
  RGBDataBlock<D> output {};

  for (size_t pixel_index = 0; pixel_index < constpow(8, D); pixel_index++) {
    YCbCrDataUnit Y  = input_Y[pixel_index];
    YCbCrDataUnit Cb = input_Cb[pixel_index] - 128;
    YCbCrDataUnit Cr = input_Cr[pixel_index] - 128;

    output[pixel_index].r = clamp(Y +                      (1.402 * Cr), 0.0, 255.0);
    output[pixel_index].g = clamp(Y - (0.344136 * Cb) - (0.714136 * Cr), 0.0, 255.0);
    output[pixel_index].b = clamp(Y + (1.772    * Cb)                  , 0.0, 255.0);
  }

  return output;
}

#endif
