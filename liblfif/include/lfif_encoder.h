/******************************************************************************\
* SOUBOR: lfif_encoder.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_ENCODER_H
#define LFIF_ENCODER_H

#include <cstdint>

#include <iosfwd>

template<size_t D, typename T>
int LFIFCompress(const T *rgb_data, const uint64_t img_dims[D+1], uint8_t quality, T max_rgb_value, std::ofstream &output);

#endif
