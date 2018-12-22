#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include "lfif.h"
#include "lfif_decoder.h"

#include <endian.h>

#include <string>
#include <fstream>

using namespace std;

void print_usage(const char *argv0);

bool parse_args(int argc, char *argv[], const char *&input_file_name, const char *&output_file_mask);

bool savePPMs(const RGBData &rgb_data, uint64_t width, uint64_t height, uint32_t color_image_count, uint64_t image_count, const string &output_file_mask);

#endif
