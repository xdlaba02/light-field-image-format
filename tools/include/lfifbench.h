/******************************************************************************\
* SOUBOR: lfifbench.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIFBENCH_H
#define LFIFBENCH_H

#include <lfiflib.h>
#include <lfif_decoder.h>

#include <cstdlib>

#include <iostream>

template<size_t D>
int decode(const char *filename, std::vector<uint8_t> &rgb_data, uint64_t img_dims[D+1]) {
  int errcode = LFIFDecompress<D>(filename, rgb_data, img_dims);

  switch (errcode) {
    case -1:
      std::cerr << "ERROR: UNABLE TO OPEN FILE \"" << filename << "\" FOR READING" << std::endl;
      return 3;
    break;

    case -2:
      std::cerr << "ERROR: MAGIC NUMBER MISMATCH" << std::endl;
      return 3;
    break;

    default:
    break;
  }

  return 0;
}

#endif
