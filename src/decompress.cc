#include "decompress.h"

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

bool savePPMs(const string &output_file_mask, const uint64_t width, const uint64_t height, const uint64_t depth, RGBData &rgb_data) {
  vector<size_t> mask_indexes {};

  for (size_t i = 0; output_file_mask[i] != '\0'; i++) {
    if (output_file_mask[i] == '#') {
      mask_indexes.push_back(i);
    }
  }

  for (size_t image = 0; image < depth; image++) {
    stringstream image_number {};
    image_number << setw(mask_indexes.size()) << setfill('0') << to_string(image);

    for (size_t index = 0; index < mask_indexes.size(); index++) {
      output_file_mask[mask_indexes[index]] = image_number.str()[index];
    }

    ofstream output(output_file_mask);
    if (output.fail()) {
      return false;
    }

    writePPM(output, width, height, rgb_data.data() + image * width * height * 3);
  }
}
