/******************************************************************************\
* SOUBOR: compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"
#include "file_mask.h"

#include <ppm.h>

#include <getopt.h>

#include <iostream>

void print_usage(const char *argv0) {
  std::cerr << "Usage: " << std::endl;
  std::cerr << argv0 << " -i <file-mask> -o <file> -q <quality>" << std::endl;
}

bool parse_args(int argc, char *argv[], const char *&input_file_mask, const char *&output_file_name, uint8_t &quality) {
  const char *arg_quality {};

  char opt;
  while ((opt = getopt(argc, argv, "i:o:q:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
          continue;
        }
        break;

      case 'o':
        if (!output_file_name) {
          output_file_name = optarg;
          continue;
        }
        break;

      case 'q':
        if (!arg_quality) {
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

  if ((!input_file_mask) || (!output_file_name) || (!arg_quality)) {
    print_usage(argv[0]);
    return false;
  }

  int tmp_quality = atoi(arg_quality);
  if ((tmp_quality < 1) || (tmp_quality > 100)) {
    print_usage(argv[0]);
    return false;
  }

  quality = tmp_quality;

  return true;
}

bool loadPPMs(const char *input_file_mask, vector<uint8_t> &rgb_data, uint64_t &width, uint64_t &height, uint32_t &color_depth, uint64_t &image_count) {
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

  for (size_t i = 0; i * i <= image_count; i++) {
    if (i * i == image_count) {
      return true;
    }
  }

  cerr << "ERROR: NOT SQUARE" << endl;
  return false;
}
