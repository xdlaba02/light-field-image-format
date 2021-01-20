/******************************************************************************\
* SOUBOR: compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"

#include <iostream>
#include <sstream>

void print_usage(const char *argv0) {
  std::cerr << "Usage: \n";
  std::cerr << argv0 << " -i <file-mask> -o <file> -q <quality> {-p} {-s}\n";
}

bool parse_args(int argc, char *argv[], const char *&input_file_mask, const char *&output_file_name, float &quality, bool &predict, bool &shift) {
  std::stringstream arg_quality {};

  input_file_mask  = nullptr;
  output_file_name = nullptr;
  quality          = 0.f;
  predict          = false;
  shift            = false;

  char opt;
  while ((opt = getopt(argc, argv, "i:o:q:ps")) >= 0) {
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
          arg_quality << optarg;
          continue;
        }
        break;

      case 'p':
        if (!predict) {
          predict = true;
        }
        continue;

      case 's':
        if (!shift) {
          shift = true;
        }
        continue;

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

  arg_quality >> quality;
  if (!arg_quality || (quality < 1.f) || (quality > 100.f)) {
    quality = 0.f;
    print_usage(argv[0]);
    return false;
  }

  return true;
}
