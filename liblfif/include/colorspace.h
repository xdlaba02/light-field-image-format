/******************************************************************************\
* SOUBOR: colorspace.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef COLORSPACE_H
#define COLORSPACE_H

#include "lfiftypes.h"

inline YCbCrUnit RGBToY(double R, double G, double B) {
  return (0.299 * R) + (0.587 * G) + (0.114 * B);
}

inline YCbCrUnit RGBToCb(double R, double G, double B) {
  return 128 - (0.168736 * R) - (0.331264 * G) + (0.5 * B);
}

inline YCbCrUnit RGBToCr(double R, double G, double B) {
  return 128 + (0.5 * R) - (0.418688 * G) - (0.081312 * B);
}

//FIXME Tady nekde nejspis ztracim psnr
inline void YtoRGB(RGBPixel<double> &rgb, YCbCrUnit Y) {
  rgb.r = Y;
  rgb.g = Y;
  rgb.b = Y;
}

inline void CbtoRGB(RGBPixel<double> &rgb, YCbCrUnit Cb) {
  rgb.g -= 0.344136 * (Cb - 128);
  rgb.b += 1.772 * (Cb - 128);
}

inline void CrtoRGB(RGBPixel<double> &rgb, YCbCrUnit Cr) {
  rgb.r += 1.402 * (Cr - 128);
  rgb.g -= 0.714136 * (Cr - 128);
}
#endif
