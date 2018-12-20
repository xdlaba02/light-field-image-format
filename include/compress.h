#ifndef COMPRESS_H
#define COMPRESS_H

#include "lfif.h"

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>

using namespace std;

void print_usage(const char *argv0);

bool parse_args(int argc, char *argv[], string &input_file_mask, string &output_file_name, uint8_t &quality);

vector<size_t> getMaskIndexes(const string &file_mask);

bool loadPPMs(string input_file_mask, uint64_t &width, uint64_t &height, uint64_t &image_count, vector<uint8_t> &rgb_data);

void writeMagicNumber(const char *number, ofstream &output);

void writeDimension(uint64_t dim, ofstream &output);

template<size_t D>
void writeQuantTable(const QuantTable<D> &table, ofstream &output) {
  output.write(reinterpret_cast<const char *>(table.data()), table.size() * sizeof(QuantTable<0>::value_type));
}

template<size_t D>
void writeTraversalTable(const TraversalTable<D> &table, ofstream &output) {
  output.write(reinterpret_cast<const char *>(table.data()), table.size() * sizeof(TraversalTable<0>::value_type));
}


#endif
