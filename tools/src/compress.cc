/******************************************************************************\
* SOUBOR: compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"

#include <iostream>
#include <sstream>

void print_usage(const char *argv0) {
  std::cerr << "Usage: \n";
  std::cerr << argv0 << " -i <file-mask> -o <file> -d <distortion> {-p} {-s}\n";
}

bool parse_args(int argc, char *argv[], const char *&input_file_mask, const char *&output_file_name, uint8_t &distortion, bool &predict, bool &shift) {

  input_file_mask  = nullptr;
  output_file_name = nullptr;
  distortion       = 0;
  predict          = false;
  shift            = false;

  bool dist_set    = false;

  char opt;
  while ((opt = getopt(argc, argv, "i:o:d:ps")) >= 0) {
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

      case 'd':
        if (!dist_set) {
          std::stringstream arg_dist {};
          arg_dist << optarg;

          uint64_t dist {};
          arg_dist >> dist;

          distortion = dist;

          if (!arg_dist) {
            continue;
          }

          dist_set = true;
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

  if ((!input_file_mask) || (!output_file_name)) {
    print_usage(argv[0]);
    return false;
  }

  return true;
}
