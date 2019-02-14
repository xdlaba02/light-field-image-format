/******************************************************************************\
* SOUBOR: ppm.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef PPM_H
#define PPM_H

#include <cstdio>
#include <cstdint>

struct PPMFileStruct {
  FILE *file;
  uint64_t width;
  uint64_t height;
  uint32_t color_depth;
};

struct Pixel {
  uint16_t r;
  uint16_t g;
  uint16_t b;
};

Pixel *allocPPMRow(size_t width);
void freePPMRow(Pixel *row);

int readPPMHeader(PPMFileStruct *ppm);
int readPPMRow(PPMFileStruct *ppm, Pixel *buffer);

int writePPMHeader(const PPMFileStruct *ppm);
int writePPMRow(PPMFileStruct *ppm, const Pixel *buffer);

#endif
