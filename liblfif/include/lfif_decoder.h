/******************************************************************************\
* SOUBOR: lfif_decoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_DECODER_H
#define LFIF_DECODER_H

#include <cstdint>

#include <iosfwd>

template<size_t D, typename T>
int LFIFDecompress(std::ifstream &input, const uint64_t img_dims[D+1], T max_rgb_value, T *rgb_data);

#endif
