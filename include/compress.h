#ifndef COMPRESS_H
#define COMPRESS_H

#include <cstdint>
#include <vector>
#include <string>

using namespace std;

void print_usage(const char *argv0);

bool parse_args(int argc, char *argv[], string &input_file_mask, string &output_file_name, uint8_t &quality);

vector<size_t> getMaskIndexes(const string &file_mask);

bool loadPPMs(string input_file_mask, uint64_t &width, uint64_t &height, uint64_t &image_count, vector<uint8_t> &rgb_data);

#endif
