/******************************************************************************\
* SOUBOR: compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"

#include <iostream>

using namespace std;

void print_usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <file-mask> -o <file> -q <quality> {-h} {}" << endl;
}

bool parse_args(int argc, char *argv[], const char *&input_file_mask, const char *&output_file_name, float &quality, bool &huffman, bool &predict) {
  const char *arg_quality {};

  input_file_mask  = nullptr;
  output_file_name = nullptr;
  quality          = 0;
  huffman          = false;
  predict          = false;

  char opt;
  while ((opt = getopt(argc, argv, "i:o:q:hp")) >= 0) {
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

      case 'h':
        if (!huffman) {
          huffman = true;
        }
        continue;

      case 'p':
        if (!predict) {
          predict = true;
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

  float tmp_quality = atof(arg_quality);
  if ((tmp_quality < 1.f) || (tmp_quality > 100.f)) {
    print_usage(argv[0]);
    return false;
  }

  quality = tmp_quality;

  return true;
}
