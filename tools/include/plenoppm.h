/******************************************************************************\
* SOUBOR: plenoppm.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef PLENOPPM_H
#define PLENOPPM_H

#include <cstdint>

bool checkPPMheaders(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, uint64_t &image_count);
bool loadPPMs(const char *input_file_mask, uint8_t *rgb_data);

bool savePPMs(const char *output_file_mask, const uint8_t *rgb_data, uint64_t width, uint64_t height, uint32_t color_depth, uint64_t image_count);

#endif
