/******************************************************************************\
* SOUBOR: lfiftest.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIFTEST_H
#define LFIFTEST_H

#include <lfif_encoder.h>
#include <lfif_decoder.h>

#include <cstdlib>

#include <iostream>

using namespace std;

template<size_t D>
size_t encode(const RGBData &rgb_data, const uint64_t img_dims[D], uint32_t image_count, uint8_t quality, const char *filename) {
  int errcode = LFIFCompress<D>(rgb_data, img_dims, image_count, quality, filename);

  if (errcode) {
    cerr << "ERROR: UNABLE TO OPEN FILE \"" << filename << "\" FOR WRITITNG" << endl;
    return 0;
  }

  ifstream encoded_file(filename, ifstream::ate | ifstream::binary);
  return encoded_file.tellg();
}

template<size_t D>
int decode(const char *filename, RGBData &rgb_data, uint64_t img_dims[D], uint64_t &image_count) {
  int errcode = LFIFDecompress<D>(filename, rgb_data, img_dims, image_count);

  switch (errcode) {
    case -1:
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << filename << "\" FOR READING" << endl;
      return 3;
    break;

    case -2:
      cerr << "ERROR: MAGIC NUMBER MISMATCH" << endl;
      return 3;
    break;

    default:
    break;
  }

  return 0;
}

#endif
