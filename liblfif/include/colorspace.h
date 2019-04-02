/******************************************************************************\
* SOUBOR: colorspace.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef COLORSPACE_H
#define COLORSPACE_H

#include "lfiftypes.h"

#include <cmath>

namespace YCbCr {

inline INPUTUNIT RGBToY(RGBUNIT R, RGBUNIT G, RGBUNIT B) {
  return (0.299 * R) + (0.587 * G) + (0.114 * B);
}

inline INPUTUNIT RGBToCb(RGBUNIT R, RGBUNIT G, RGBUNIT B) {
  return - (0.168736 * R) - (0.331264 * G) + (0.5 * B);
}

inline INPUTUNIT RGBToCr(RGBUNIT R, RGBUNIT G, RGBUNIT B) {
  return (0.5 * R) - (0.418688 * G) - (0.081312 * B);
}

inline INPUTUNIT YCbCrToR(INPUTUNIT Y, INPUTUNIT, INPUTUNIT Cr) {
  return Y + 1.402 * Cr;
}

inline INPUTUNIT YCbCrToG(INPUTUNIT Y, INPUTUNIT Cb, INPUTUNIT Cr) {
  return Y - 0.344136 * Cb - 0.714136 * Cr;
}

inline INPUTUNIT YCbCrToB(INPUTUNIT Y, INPUTUNIT Cb, INPUTUNIT) {
  return Y + 1.772 * Cb;
}

}

namespace YCoCg {

inline INPUTUNIT RGBToY(RGBUNIT R, RGBUNIT G, RGBUNIT B) {
  return (0.25 * R) + (0.5 * G) + (0.25 * B);
}

inline INPUTUNIT RGBToCo(RGBUNIT R, RGBUNIT, RGBUNIT B) {
  return (0.5 * R) - (0.5 * B);
}

inline INPUTUNIT RGBToCg(RGBUNIT R, RGBUNIT G, RGBUNIT B) {
  return - (0.25 * R) + (0.5 * G) - (0.25 * B);
}

inline INPUTUNIT YCoCgToR(INPUTUNIT Y, INPUTUNIT Co, INPUTUNIT Cg) {
  return Y + Co - Cg;
}

inline INPUTUNIT YCoCgToG(INPUTUNIT Y, INPUTUNIT, INPUTUNIT Cg) {
  return Y + Cg;
}

inline INPUTUNIT YCoCgToB(INPUTUNIT Y, INPUTUNIT Co, INPUTUNIT Cg) {
  return Y - Co - Cg;
}

}
#endif
