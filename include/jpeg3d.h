/*******************************************************\
* SOUBOR: jpeg3d.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 22. 10. 2018
\*******************************************************/

#ifndef JPEG3D_H
#define JPEG3D_H

#include <stdint.h>

#include <vector>

using namespace std;

bool RGBtoJPEG3D(const char *output_filename, const vector<uint8_t> &rgb_data, const uint64_t w, const uint64_t h, const uint64_t ix, const uint64_t iy, const uint8_t quality) {};
bool JPEG3DtoRGB(const char *input_filename, uint64_t &w, uint64_t &h, uint64_t &ix, uint64_t &iy, vector<uint8_t> &rgb_data) {};

#endif
