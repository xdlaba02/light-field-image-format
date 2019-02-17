/******************************************************************************\
* SOUBOR: lfiflib.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIFLIB_H
#define LFIFLIB_H

#include <cstdint>

typedef enum {
  LFIF_2D,
  LFIF_3D,
  LFIF_4D
} CompressMethod;

typedef enum {
  RGB24,
  RGB48,
} ColorSpace;

typedef struct {
  uint64_t image_width;
  uint64_t image_height;
  uint64_t image_count;
  uint8_t  quality;

  CompressMethod method;
  ColorSpace color_space;

  union {
    uint8_t  *rgb_data_24;
    uint16_t *rgb_data_48;
  };

} LFIFCompressStruct;

//int LFIFCompress(const LFIFCompressStruct *lfif_compress_struct, char *output_file_name);

#endif
