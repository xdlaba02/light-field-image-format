#include "compress.h"
#include "ppm.h"

#include <getopt.h>

#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

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

vector<size_t> getMaskIndexes(const string &file_mask) {
  vector<size_t> mask_indexes {};
  for (size_t i = 0; file_mask[i] != '\0'; i++) {
    if (file_mask[i] == '#') {
      mask_indexes.push_back(i);
    }
  }
  return mask_indexes;
}

bool loadPPMs(string input_file_mask, uint64_t &width, uint64_t &height, uint64_t &image_count, vector<uint8_t> &rgb_data) {
  vector<size_t> mask_indexes = getMaskIndexes(input_file_mask);

  for (size_t image = 0; image < pow(10, mask_indexes.size()); image++) {
    stringstream image_number {};
    image_number << setw(mask_indexes.size()) << setfill('0') << to_string(image);

    for (size_t index = 0; index < mask_indexes.size(); index++) {
      input_file_mask[mask_indexes[index]] = image_number.str()[index];
    }

    ifstream input(input_file_mask);
    if (input.fail()) {
      continue;
    }

    image_count++;

    uint64_t image_width {};
    uint64_t image_height {};

    if (!readPPM(input, image_width, image_height, rgb_data)) {
      cerr << "ERROR: BAD PPM" << endl;
      return false;
    }

    if (width && height) {
      if ((image_width != width) || (image_height != height)) {
        cerr << "ERROR: WIDTHS NOT SAME" << endl;
        return false;
      }
    }

    width = image_width;
    height = image_height;
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
