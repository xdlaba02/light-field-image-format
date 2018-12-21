#include "compress.h"
#include "ppm.h"
#include "zigzag.h"
#include "file_mask.h"

#include <getopt.h>

#include <cmath>
#include <iostream>

void print_usage(const char *argv0) {
  std::cerr << "Usage: " << std::endl;
  std::cerr << argv0 << " -i <file-mask> -o <file> -q <quality>" << std::endl;
}

bool parse_args(int argc, char *argv[], string &input_file_mask, string &output_file_name, uint8_t &quality) {
  string arg_quality {};

  char opt;
  while ((opt = getopt(argc, argv, "i:o:q:")) >= 0) {
    switch (opt) {
      case 'i':
        if (input_file_mask.empty()) {
          input_file_mask = optarg;
          continue;
        }
        break;

      case 'o':
        if (output_file_name.empty()) {
          output_file_name = optarg;
          continue;
        }
        break;

      case 'q':
        if (arg_quality.empty()) {
          arg_quality = optarg;
          continue;
        }
        break;

      default:
        break;
    }

    print_usage(argv[0]);
    return false;
  }

  if ((input_file_mask.empty()) || (output_file_name.empty()) || (arg_quality.empty())) {
    print_usage(argv[0]);
    return false;
  }

  int tmp_quality = stoi(arg_quality);
  if ((tmp_quality < 1) || (tmp_quality > 100)) {
    print_usage(argv[0]);
    return false;
  }

  quality = tmp_quality;

  return true;
}

bool loadPPMs(string input_file_mask, vector<uint8_t> &rgb_data, uint64_t &width, uint64_t &height, uint32_t &color_depth, uint64_t &image_count) {
  FileMask file_name(input_file_mask);

  for (size_t image = 0; image < file_name.count(); image++) {
    ifstream input(file_name[image]);
    if (input.fail()) {
      continue;
    }

    image_count++;

    uint64_t image_width       {};
    uint64_t image_height      {};
    uint32_t image_color_depth {};

    if (!readPPM(input, rgb_data, image_width, image_height, image_color_depth)) {
      cerr << "ERROR: BAD PPM" << endl;
      return false;
    }

    if (width && height && color_depth) {
      if ((image_width != width) || (image_height != height) || (image_color_depth != color_depth)) {
        cerr << "ERROR: DIMENSIONS NOT SAME" << endl;
        return false;
      }
    }

    width       = image_width;
    height      = image_height;
    color_depth = image_color_depth;
  }

  if (!image_count) {
    cerr << "ERROR: NO IMAGE" << endl;
    return false;
  }

  if (static_cast<uint64_t>(sqrt(image_count) * sqrt(image_count)) != image_count) {
    cerr << "ERROR: NOT SQUARE" << endl;
    return false;
  }

  return true;
}

void writeMagicNumber(const char *number, ofstream &output) {
  output.write(number, 8);
}

void writeDimension(uint64_t dim, ofstream &output) {
  uint64_t raw = htobe64(dim);
  output.write(reinterpret_cast<char *>(&raw),  sizeof(raw));
}

RGBData zigzagShift(const RGBData &rgb_data, uint64_t image_count) {
  RGBData zigzag_data(rgb_data.size());

  size_t image_size = rgb_data.size() / image_count;

  vector<size_t> zigzag_table = generateZigzagTable(sqrt(image_count));

  for (size_t i = 0; i < image_count; i++) {
    for (size_t j = 0; j < image_size; j++) {
      zigzag_data[zigzag_table[i] * image_size + j] = rgb_data[i * image_size + j];
    }
  }

  return zigzag_data;
}
