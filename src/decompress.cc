#include "decompress.h"
#include "ppm.h"
#include "zigzag.h"
#include "file_mask.h"

#include <getopt.h>

#include <iostream>

void print_usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <file> -o <file-mask>" << endl;
}

bool parse_args(int argc, char *argv[], string &input_file_name, string &output_file_mask) {
  char opt;
  while ((opt = getopt(argc, argv, "i:o:q:")) >= 0) {
    switch (opt) {
      case 'i':
        if (input_file_name.empty()) {
          input_file_name = optarg;
          continue;
        }
        break;

      case 'o':
        if (output_file_mask.empty()) {
          output_file_mask = optarg;
          continue;
        }
        break;

      default:
        break;
    }

    print_usage(argv[0]);
    return false;
  }

  if ((input_file_name.empty()) || (output_file_mask.empty())) {
    print_usage(argv[0]);
    return false;
  }

  return true;
}

bool checkMagicNumber(const string &cmp, ifstream &input) {
  char magic_number[9] {};
  input.read(magic_number, 8);

  if (string(magic_number) != cmp) {
    return false;
  }
  else {
    return true;
  }
}

uint64_t readDimension(ifstream &input) {
  uint64_t raw {};
  input.read(reinterpret_cast<char *>(&raw), sizeof(uint64_t));
  return fromBigEndian(raw);
}

bool savePPMs(const RGBData &rgb_data, uint64_t width, uint64_t height, uint32_t color_depth, uint64_t image_count, const string &output_file_mask) {
  size_t image_size = width * height * 3;

  if (color_depth >= 256) {
    image_size *= 2;
  }

  FileMask file_name(output_file_mask);

  for (size_t image = 0; image < image_count; image++) {
    ofstream output(file_name[image]);
    if (output.fail()) {
      return false;
    }

    writePPM(rgb_data.data() + image * image_size, width, height, color_depth, output);
  }

  return true;
}

RGBData zigzagDeshift(const RGBData &rgb_data, uint64_t image_count) {
  RGBData zigzag_data(rgb_data.size());

  size_t image_size = rgb_data.size() / image_count;

  vector<size_t> zigzag_table = generateZigzagTable(sqrt(image_count));

  for (size_t i = 0; i < image_count; i++) {
    for (size_t j = 0; j < image_size; j++) {
      zigzag_data[i * image_size + j] = rgb_data[zigzag_table[i] * image_size + j];
    }
  }

  return zigzag_data;
}
