/******************************************************************************\
* SOUBOR: decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "decompress.h"
#include "file_mask.h"

#include <ppm.h>

#include <getopt.h>

#include <iostream>

void print_usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <file> -o <file-mask>" << endl;
}

bool parse_args(int argc, char *argv[], const char *&input_file_name, const char *&output_file_mask) {
  char opt;
  while ((opt = getopt(argc, argv, "i:o:q:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_name) {
          input_file_name = optarg;
          continue;
        }
        break;

      case 'o':
        if (!output_file_mask) {
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

  if ((!input_file_name) || (!output_file_mask)) {
    print_usage(argv[0]);
    return false;
  }

  return true;
}

bool savePPMs(const vector<uint8_t> &rgb_data, uint64_t width, uint64_t height, uint32_t color_depth, uint64_t image_count, const string &output_file_mask) {
  size_t image_size = width * height * 3;

  if (color_depth >= 256) {
    image_size *= 2;
  }

  size_t pos = output_file_mask.find_last_of('/');

  FileMask file_name(output_file_mask);

  for (size_t image = 0; image < image_count; image++) {
    string command("mkdir -p " + file_name[image].substr(0, pos));
    system(command.c_str());
    ofstream output(file_name[image]);
    if (output.fail()) {
      return false;
    }

    writePPM(rgb_data.data() + image * image_size, width, height, color_depth, output);
  }

  return true;
}
