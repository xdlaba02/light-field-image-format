/*******************************************************\
* SOUBOR: main.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "jpeg_encoder.h"
#include "jpeg_decoder.h"
#include "ppm.h"
#include "dct.h"

#include <iostream>

using namespace std;

void print_usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -e <2D|3D|4D> <1..100> <no. images on x axis> path/to/output path/to/inputs*.ppm" << endl;
  cerr << argv0 << " -d path/to/output/prefix path/to/input.jpeg*d" << endl;
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

vector<uint8_t> zigzagShift(const vector<uint8_t> &rgb_data, uint64_t ix, uint64_t iy) {
  vector<uint8_t> rgb_shifted {};

  uint64_t size_2d = rgb_data.size() / (ix * iy);

  uint64_t x = 0;
  uint64_t y = 0;
  auto index = [&]() -> unsigned { return y*size_2d*ix + x*size_2d; };

  while (true) {
    rgb_shifted.insert(rgb_shifted.end(), rgb_data.begin() + index(), rgb_data.begin() + index() + size_2d);

    if (x < ix - 1) {
      x++;
    }
    else if (y < iy - 1) {
      y++;
    }
    else {
      break;
    }

    while ((x > 0) && (y < iy - 1)) {
      rgb_shifted.insert(rgb_shifted.end(), rgb_data.begin() + index(), rgb_data.begin() + index() + size_2d);
      x--;
      y++;
    }

    rgb_shifted.insert(rgb_shifted.end(), rgb_data.begin() + index(), rgb_data.begin() + index() + size_2d);

    if (y < iy - 1) {
      y++;
    }
    else if (x < ix - 1) {
      x++;
    }
    else {
      break;
    }

    while ((x < ix - 1) && (y > 0)) {
      rgb_shifted.insert(rgb_shifted.end(), rgb_data.begin() + index(), rgb_data.begin() + index() + size_2d);
      x++;
      y--;
    }
  }

  return rgb_shifted;
}

vector<uint8_t> zigzagDeshift(const vector<uint8_t> &shifted_data, uint64_t ix, uint64_t iy) {
  vector<uint8_t> rgb_data(shifted_data.size());

  uint64_t size_2d = shifted_data.size() / (ix * iy);

  uint64_t shift_index = 0;
  uint64_t x = 0;
  uint64_t y = 0;
  auto index = [&]() -> unsigned { return y*size_2d*ix + x*size_2d; };

  while (true) {
    for (uint64_t i = 0; i < size_2d; i++) {
      rgb_data[index()+i] = shifted_data[shift_index++];
    }

    if (x < ix - 1) {
      x++;
    }
    else if (y < iy - 1) {
      y++;
    }
    else {
      break;
    }

    while ((x > 0) && (y < iy - 1)) {
      for (uint64_t i = 0; i < size_2d; i++) {
        rgb_data[index()+i] = shifted_data[shift_index++];
      }
      x--;
      y++;
    }

    for (uint64_t i = 0; i < size_2d; i++) {
      rgb_data[index()+i] = shifted_data[shift_index++];
    }

    if (y < iy - 1) {
      y++;
    }
    else if (x < ix - 1) {
      x++;
    }
    else {
      break;
    }

    while ((x < ix - 1) && (y > 0)) {
      for (uint64_t i = 0; i < size_2d; i++) {
        rgb_data[index()+i] = shifted_data[shift_index++];
      }
      x++;
      y--;
    }
  }

  return rgb_data;
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
      if (!RGBtoJPEG<2>(output_filename, rgb_data, {width, height, count_x, count_y}, quality)) {
        return -3;
      }
    }
    else if (method == "3D") {
      vector<uint8_t> rgb_shifted = zigzagShift(rgb_data, count_x, count_y);
      if (!RGBtoJPEG<3>(output_filename, rgb_shifted, {width, height, count_x, count_y}, quality)) {
        return -3;
      }
    }
    else if (method == "4D") {
      if (!RGBtoJPEG<4>(output_filename, rgb_data, {width, height, count_x, count_y}, quality)) {
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

    if (input_filename.substr(input_filename.find_last_of(".") + 1) == "jpeg2d") {
      if (!JPEGtoRGB<2>(argv[3], dimensions, rgb_data)) {
        return -2;
      }
    }
    else if (input_filename.substr(input_filename.find_last_of(".") + 1) == "jpeg3d") {
      vector<uint8_t> rgb_shifted {};
      if (!JPEGtoRGB<3>(argv[3], dimensions, rgb_shifted)) {
        return -2;
      }
      rgb_data = zigzagDeshift(rgb_shifted, dimensions[2], dimensions[3]);
    }
    else if (input_filename.substr(input_filename.find_last_of(".") + 1) == "jpeg4d") {
      if (!JPEGtoRGB<4>(argv[3], dimensions, rgb_data)) {
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
