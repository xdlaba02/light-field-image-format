/******************************************************************************\
* SOUBOR: plenoppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"
#include "file_mask.h"

#include <ppm.h>

#include <iostream>

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

    if (readPPM(input, rgb_data, image_width, image_height, image_color_depth)) {
      cerr << "ERROR: BAD PPM" << endl;
      return false;
    }

    if (width && height && color_depth) {
      if ((image_width != width) || (image_height != height) || (image_color_depth != color_depth)) {
        cerr << "ERROR: DIMENSIONS MISMATCH" << endl;
        return false;
      }
    }

    width       = image_width;
    height      = image_height;
    color_depth = image_color_depth;
  }

  if (!image_count) {
    cerr << "ERROR: NO IMAGE LOADED" << endl;
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
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << file_name[image] << "\" FOR WRITING" << endl;
      return false;
    }

    if (writePPM(rgb_data.data() + image * image_size, width, height, color_depth, output)) {
      cerr << "ERROR: UNABLE TO WRITE PPM FILE" << endl;
      return false;
    }
  }

  return true;
}
