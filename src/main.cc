/*******************************************************\
* SOUBOR: main.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "jpeg2d.h"
#include "jpeg3d.h"
#include "jpeg4d.h"
#include "ppm.h"

#include <iostream>

using namespace std;

void print_usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -e <2D|3D|4D> <1..100> <no. images on x axis> path/to/output.jpeg*d path/to/inputs*.ppm" << endl;
  cerr << argv0 << " -d <2D|3D|4D> path/to/output/prefix path/to/input.jpeg*d" << endl;
}

bool loadMultiplePPMs(int filesc, char *filesv[], uint64_t &width, uint64_t &height, vector<uint8_t> &rgb_data) {
  uint64_t prev_width = 0;
  uint64_t prev_height = 0;

  for (int64_t i = 0; i < filesc; i++) {
    if (!loadPPM(filesv[i], width, height, rgb_data)) {
      cerr << "Unable to load image " << filesv[i] << "!" << endl;
      return false;
    }
    if (prev_width != 0 && prev_height != 0 && ((prev_width != width) || (prev_height != height))) {
      cerr << "Image sizes do not match!" << endl;
      return false;
    }

    prev_width = width;
    prev_height = height;
  }

  return true;
}

int main(int argc, char *argv[]) {
  if (argc < 5) {
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
      if (!RGBtoJPEG2D(output_filename, rgb_data, width, height, count_x, count_y, quality)) {
        return -3;
      }
    }
    else if (method == "3D") {
      if (!RGBtoJPEG3D(output_filename, rgb_data, width, height, count_x, count_y, quality)) {
        return -3;
      }
    }
    else if (method == "4D") {
      if (!RGBtoJPEG4D(output_filename, rgb_data, width, height, count_x, count_y, quality)) {
        return -3;
      }
    }
    else {
      print_usage(argv[0]);
      return -1;
    }
  }
  else if (type == "-d" || type == "--decode") {
    if (argc != 5) {
      print_usage(argv[0]);
      return -1;
    }

    const string method(argv[2]);

    const string output_filename(argv[3]);
    if (method == "2D") {
      if (!JPEG2DtoRGB(argv[4], width, height, count_x, count_y, rgb_data)) {
        return -2;
      }
    }
    else if (method == "3D") {
      if (!JPEG3DtoRGB(argv[4], width, height, count_x, count_y, rgb_data)) {
        return -2;
      }
    }
    else if (method == "4D") {
      if (!JPEG4DtoRGB(argv[4], width, height, count_x, count_y, rgb_data)) {
        return -2;
      }
    }
    else {
      print_usage(argv[0]);
      return -1;
    }

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
