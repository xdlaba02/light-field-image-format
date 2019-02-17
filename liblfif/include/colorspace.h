/******************************************************************************\
* SOUBOR: colorspace.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef COLORSPACE_H
#define COLORSPACE_H

#include "lfiftypes.h"

inline YCbCrUnit8 RGBToY(RGBUnit8 R, RGBUnit8 G, RGBUnit8 B) {
  return (0.299 * R) + (0.587 * G) + (0.114 * B);
}

inline YCbCrUnit8 RGBToCb(RGBUnit8 R, RGBUnit8 G, RGBUnit8 B) {
  return 128 - (0.168736 * R) - (0.331264 * G) + (0.5 * B);
}

inline YCbCrUnit8 RGBToCr(RGBUnit8 R, RGBUnit8 G, RGBUnit8 B) {
  return 128 + (0.5 * R) - (0.418688 * G) - (0.081312 * B);
}

inline void YtoRGB(RGBPixel<double> &rgb, YCbCrUnit8 Y) {
  rgb.r = Y;
  rgb.g = Y;
  rgb.b = Y;
}

inline void CbtoRGB(RGBPixel<double> &rgb, YCbCrUnit8 Cb) {
  rgb.g -= 0.344136 * (Cb - 128);
  rgb.b += 1.772 * (Cb - 128);
}

inline void CrtoRGB(RGBPixel<double> &rgb, YCbCrUnit8 Cr) {
  rgb.r += 1.402 * (Cr - 128);
  rgb.g -= 0.714136 * (Cr - 128);
}
#endif
