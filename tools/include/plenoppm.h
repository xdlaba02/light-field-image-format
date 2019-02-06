/******************************************************************************\
* SOUBOR: plenoppm.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef PLENOPPM_H
#define PLENOPPM_H

#include <cstdint>
#include <vector>
#include <string>

using namespace std;

bool loadPPMs(const char *input_file_mask, vector<uint8_t> &rgb_data, uint64_t &width, uint64_t &height, uint32_t &color_depth, uint64_t &image_count);
bool savePPMs(const vector<uint8_t> &rgb_data, uint64_t width, uint64_t height, uint32_t color_image_count, uint64_t image_count, const string &output_file_mask);

#endif
