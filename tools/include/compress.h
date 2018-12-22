/******************************************************************************\
* SOUBOR: compress.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef COMPRESS_H
#define COMPRESS_H

#include <cstdint>
#include <vector>

using namespace std;

void print_usage(const char *argv0);

bool parse_args(int argc, char *argv[], const char *&input_file_mask, const char *&output_file_name, uint8_t &quality);

bool loadPPMs(const char *input_file_mask, vector<uint8_t> &rgb_data, uint64_t &width, uint64_t &height, uint32_t &color_depth, uint64_t &image_count);

#endif
