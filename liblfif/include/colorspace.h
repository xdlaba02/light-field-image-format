/******************************************************************************\
* SOUBOR: colorspace.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef COLORSPACE_H
#define COLORSPACE_H

#include "lfiftypes.h"

#include <cmath>

inline YCBCRUNIT RGBToY(uint16_t R, uint16_t G, uint16_t B) {
  return (0.299 * R) + (0.587 * G) + (0.114 * B);
}

inline YCBCRUNIT RGBToCb(uint16_t R, uint16_t G, uint16_t B) {
  return (-0.168736 * R) - (0.331264 * G) + (0.5 * B);
}

inline YCBCRUNIT RGBToCr(uint16_t R, uint16_t G, uint16_t B) {
  return (0.5 * R) - (0.418688 * G) - (0.081312 * B);
}

inline void YtoRGB(RGBPixel<double> &rgb, YCBCRUNIT Y, uint16_t) {
  rgb.r = Y;
  rgb.g = Y;
  rgb.b = Y;
}

inline void CbtoRGB(RGBPixel<double> &rgb, YCBCRUNIT Cb, uint16_t max_rgb_value) {
  rgb.g -= 0.344136 * (Cb - ((max_rgb_value + 1) / 2));
  rgb.b += 1.772    * (Cb - ((max_rgb_value + 1) / 2));
}

inline void CrtoRGB(RGBPixel<double> &rgb, YCBCRUNIT Cr, uint16_t max_rgb_value) {
  rgb.r += 1.402    * (Cr - ((max_rgb_value + 1) / 2));
  rgb.g -= 0.714136 * (Cr - ((max_rgb_value + 1) / 2));
}
#endif
