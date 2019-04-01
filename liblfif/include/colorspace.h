/******************************************************************************\
* SOUBOR: colorspace.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef COLORSPACE_H
#define COLORSPACE_H

#include "lfiftypes.h"

#include <cmath>

inline YCBCRUNIT RGBToY(RGBUNIT R, RGBUNIT G, RGBUNIT B) {
  return (0.299 * R) + (0.587 * G) + (0.114 * B);
}

inline YCBCRUNIT RGBToCb(RGBUNIT R, RGBUNIT G, RGBUNIT B) {
  return - (0.168736 * R) - (0.331264 * G) + (0.5 * B);
}

inline YCBCRUNIT RGBToCr(RGBUNIT R, RGBUNIT G, RGBUNIT B) {
  return (0.5 * R) - (0.418688 * G) - (0.081312 * B);
}

inline YCBCRUNIT YCbCrToR(YCBCRUNIT Y, YCBCRUNIT, YCBCRUNIT Cr) {
  return Y + 1.402 * Cr;
}

inline YCBCRUNIT YCbCrToG(YCBCRUNIT Y, YCBCRUNIT Cb, YCBCRUNIT Cr) {
  return Y - 0.344136 * Cb - 0.714136 * Cr;
}

inline YCBCRUNIT YCbCrToB(YCBCRUNIT Y, YCBCRUNIT Cb, YCBCRUNIT) {
  return Y + 1.772 * Cb;
}
#endif
