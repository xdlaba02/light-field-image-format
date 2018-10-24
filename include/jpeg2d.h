/*******************************************************\
* SOUBOR: jpeg2d.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 22. 10. 2018
\*******************************************************/

#ifndef JPEG2D_H
#define JPEG2D_H

#include <stdint.h>

#include <vector>

using namespace std;

bool RGBtoJPEG2D(const char *output_filename, const vector<uint8_t> &rgb_data, const uint64_t w, const uint64_t h, const uint64_t ix, const uint64_t iy, const uint8_t quality);
bool JPEG2DtoRGB(const char *input_filename, uint64_t &w, uint64_t &h, uint64_t &ix, uint64_t &iy, vector<uint8_t> &rgb_data);

#endif
