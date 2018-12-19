/*******************************************************\
* SOUBOR: main.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "lfif_encoder.h"
#include "lfif_decoder.h"
#include "ppm.h"
#include "dct.h"

#include <iostream>

using namespace std;

void print_usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -e <2D|3D|4D> <1..100> <no. images on x axis> path/to/output path/to/inputs*.ppm" << endl;
  cerr << argv0 << " -d path/to/output/prefix path/to/input.lfif*d" << endl;
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    print_usage(argv[0]);
    return -1;
  }

  vector<uint8_t> rgb_data {};
  uint64_t width           {};
  uint64_t height          {};
  uint64_t count_x         {};
  uint64_t count_y         {};

  const string type(argv[1]);

  if (type == "-e" || type == "--encode") {
    if (argc < 7) {
      print_usage(argv[0]);
      return -1;
    }

    const string method(argv[2]);

    int8_t quality {static_cast<int8_t>(atoi(argv[3]))};
    if ((quality < 1) || (quality > 100)) {
      print_usage(argv[0]);
      return -1;
    }

    int64_t tmp = atoi(argv[4]);
    if (tmp < 1) {
      print_usage(argv[0]);
      return -1;
    }

    count_x = tmp;

    if ((argc - 6) % count_x) {
      cerr << "Pocet obrazku neni delitelny sirkou " << static_cast<long>(count_x) << "!" << endl;
      return -2;
    }

    count_y = (argc - 6)/count_x;

    if (!loadMultiplePPMs(argc - 6, argv + 6, width, height, rgb_data)) {
      return -2;
    }

    const char *output_filename {argv[5]};

    if (method == "2D") {
      if (!RGBtoLFIF<2>(output_filename, rgb_data, {width, height, count_x, count_y}, quality)) {
        return -3;
      }
    }
    else if (method == "3D") {
      vector<uint8_t> rgb_shifted = zigzagShift(rgb_data, count_x, count_y);
      if (!RGBtoLFIF<3>(output_filename, rgb_shifted, {width, height, count_x, count_y}, quality)) {
        return -3;
      }
    }
    else if (method == "4D") {
      if (!RGBtoLFIF<4>(output_filename, rgb_data, {width, height, count_x, count_y}, quality)) {
        return -3;
      }
    }
    else {
      print_usage(argv[0]);
      return -1;
    }
  }
  else if (type == "-d" || type == "--decode") {
    if (argc != 4) {
      print_usage(argv[0]);
      return -1;
    }

    vector<uint64_t> dimensions(4);

    const string output_filename(argv[2]);
    const string input_filename(argv[3]);

    if (input_filename.substr(input_filename.find_last_of(".") + 1) == "lfif2d") {
      if (!LFIFtoRGB<2>(argv[3], dimensions, rgb_data)) {
        return -2;
      }
    }
    else if (input_filename.substr(input_filename.find_last_of(".") + 1) == "lfif3d") {
      vector<uint8_t> rgb_shifted {};
      if (!LFIFtoRGB<3>(argv[3], dimensions, rgb_shifted)) {
        return -2;
      }
      rgb_data = zigzagDeshift(rgb_shifted, dimensions[2], dimensions[3]);
    }
    else if (input_filename.substr(input_filename.find_last_of(".") + 1) == "lfif4d") {
      if (!LFIFtoRGB<4>(argv[3], dimensions, rgb_data)) {
        return -2;
      }
    }
    else {
      print_usage(argv[0]);
      return -1;
    }

    width = dimensions[0];
    height = dimensions[1];
    count_x = dimensions[2];
    count_y = dimensions[3];

    for (uint64_t y = 0; y < count_y; y++) {
      for (uint64_t x = 0; x < count_x; x++) {
        if (!savePPM(output_filename + "_" + to_string(y) + "_" + to_string(x) + ".ppm", width, height, rgb_data.data() + y*count_x*width*height*3 + x*width*height*3)) {
          return -3;
        }
      }
    }
  }
  else {
    print_usage(argv[0]);
    return -1;
  }

  return 0;
}
