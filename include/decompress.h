#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include "lfif.h"

#include <string>
#include <fstream>

using namespace std;

void print_usage(const char *argv0);

bool parse_args(int argc, char *argv[], string &input_file_name, string &output_file_mask);

bool checkMagicNumber(const string &cmp, ifstream &input);

uint64_t readDimension(ifstream &input);

bool savePPMs(const string &output_file_mask, const uint64_t width, const uint64_t height, const uint64_t depth, RGBData &rgb_data);

template<size_t D>
QuantTable<D> readQuantTable(ifstream &input) {
  QuantTable<D> table {};
  input.read(reinterpret_cast<char *>(table.data()), table.size() * sizeof(QuantTable<0>::value_type));
  return table;
}

template<size_t D>
TraversalTable<D> readTraversalTable(ifstream &input) {
  TraversalTable<D> table {};
  input.read(reinterpret_cast<char *>(table.data()), table.size() * sizeof(TraversalTable<0>::value_type));
  return table;
}

#endif
