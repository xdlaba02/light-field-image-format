/******************************************************************************\
* SOUBOR: lfiflib.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIFLIB_H
#define LFIFLIB_H

#include <stdint.h>

typedef enum {
  LFIF_2D,
  LFIF_3D,
  LFIF_4D,
  LFIF_METHODS_CNT
} CompressMethod;

typedef enum {
  RGB24,
  RGB48,
  COLORSPACES_CNT
} ColorSpace;

typedef struct {
  CompressMethod method;
  ColorSpace     color_space;
  uint64_t       image_width;
  uint64_t       image_height;
  uint64_t       image_count;
  uint8_t        quality;
  const char    *output_file_name;
} LFIFCompressStruct;

int LFIFCompress(LFIFCompressStruct *lfif, const void *rgb_data);

typedef struct {
  CompressMethod method;
  ColorSpace     color_space;
  uint64_t       image_width;
  uint64_t       image_height;
  uint64_t       image_count;
  const char    *input_file_name;
} LFIFDecompressStruct;

int LFIFReadHeader(LFIFDecompressStruct *lfif);
int LFIFDecompress(LFIFDecompressStruct *lfif, void *rgb_buffer);


#endif
