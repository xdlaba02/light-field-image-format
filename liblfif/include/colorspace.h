/******************************************************************************\
* SOUBOR: colorspace.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef COLORSPACE_H
#define COLORSPACE_H

inline YCbCrUnit RGBToY(RGBDataUnit R, RGBDataUnit G, RGBDataUnit B) {
  return (0.299 * R) + (0.587 * G) + (0.114 * B);
}

inline YCbCrUnit RGBToCb(RGBDataUnit R, RGBDataUnit G, RGBDataUnit B) {
  return 128 - (0.168736 * R) - (0.331264 * G) + (0.5 * B);
}

inline YCbCrUnit RGBToCr(RGBDataUnit R, RGBDataUnit G, RGBDataUnit B) {
  return 128 + (0.5 * R) - (0.418688 * G) - (0.081312 * B);
}

inline void YtoRGB(RGBPixel &rgb, YCbCrUnit Y) {
  rgb.r = clamp(Y, 0.0, 255.0);
  rgb.g = clamp(Y, 0.0, 255.0);
  rgb.b = clamp(Y, 0.0, 255.0);
}

inline void CbtoRGB(RGBPixel &rgb, YCbCrUnit Cb) {
  rgb.g = clamp(rgb.g - 0.344136 * (Cb - 128), 0.0, 255.0);
  rgb.b = clamp(rgb.b + 1.772 * (Cb - 128), 0.0, 255.0);
}

inline void CrtoRGB(RGBPixel &rgb, YCbCrUnit Cr) {
  rgb.r = clamp(rgb.r + 1.402 * (Cr - 128), 0.0, 255.0);
  rgb.g = clamp(rgb.g - 0.714136 * (Cr - 128), 0.0, 255.0);
}
#endif
