/*******************************************************\
* SOUBOR: ppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "ppm.h"

#include <endian.h>

#include <iostream>

bool readPPM(ifstream &input, vector<uint8_t> &rgb_data, uint64_t &width, uint64_t &height, uint32_t &color_depth) {
  if (!parseHeader(input, width, height, color_depth)) {
    return false;
  }

  size_t image_size = width * height * 3;
  size_t original_size = rgb_data.size();

  if (color_depth < 256) {
    rgb_data.resize(original_size + image_size);
    input.read(reinterpret_cast<char *>(rgb_data.data() + original_size), image_size);
    if (input.fail()) {
      return false;
    }
  }
  else {
    rgb_data.resize(original_size + (2 * image_size));
    input.read(reinterpret_cast<char *>(rgb_data.data() + original_size), 2 * image_size);
    if (input.fail()) {
      return false;
    }
    uint16_t *arr = reinterpret_cast<uint16_t *>(rgb_data.data());
    for (size_t i = 0; i < image_size; i++) {
      arr[i] = be16toh(arr[i]);
    }
  }

  return true;
}

bool writePPM(const uint8_t *rgb_data, uint64_t width, uint64_t height, uint32_t color_depth, ofstream &output) {
  output << "P6" << endl;
  output << width << endl;
  output << height << endl;
  output << color_depth << endl;

  size_t image_size = width * height * 3;

  if (color_depth >= 256) {
    image_size *= 2;
  }

  output.write(reinterpret_cast<const char *>(rgb_data), image_size);
  if (output.fail()) {
    return false;
  }

  return true;
}

void skipUntilEol(ifstream &input) {
  char c {};
  while(input.get(c)) {
    if (c == '\n') {
      return;
    }
  }
  return;
}

bool parseHeader(ifstream &input, uint64_t &width, uint64_t &height, uint32_t &depth) {
  string str_width  {};
  string str_height {};
  string str_depth  {};

  State state = STATE_INIT;

  char c {};
  while(input.get(c)) {
    if (input.eof()) {
      return false;
    }

    switch (state) {
      case STATE_INIT:
      if (c == 'P') {
        state = STATE_P;
      }
      else {
        return false;
      }
      break;

      case STATE_P:
      if (c == '6') {
        state = STATE_P6;
      }
      else {
        return false;
      }
      break;

      case STATE_P6:
      if (c == '#') {
        skipUntilEol(input);
        state = STATE_P6_SPACE;
      }
      else if (isspace(c)) {
        state = STATE_P6_SPACE;
      }
      else {
        return false;
      }
      break;

      case STATE_P6_SPACE:
      if (c == '#') {
        skipUntilEol(input);
      }
      else if (isspace(c)) {
        // STAY HERE
      }
      else if (isdigit(c)) {
        str_width += c;
        state = STATE_WIDTH;
      }
      else {
        return false;
      }
      break;

      case STATE_WIDTH:
      if (c == '#') {
        skipUntilEol(input);
        state = STATE_WIDTH_SPACE;
      }
      else if (isspace(c)) {
        state = STATE_WIDTH_SPACE;
      }
      else if (isdigit(c)) {
        str_width += c;
      }
      else {
        return false;
      }
      break;

      case STATE_WIDTH_SPACE:
      if (c == '#') {
        skipUntilEol(input);
      }
      else if (isspace(c)) {
        // STAY HERE
      }
      else if (isdigit(c)) {
        str_height += c;
        state = STATE_HEIGHT;
      }
      else {
        return false;
      }
      break;

      case STATE_HEIGHT:
      if (c == '#') {
        skipUntilEol(input);
        state = STATE_HEIGHT_SPACE;
      }
      else if (isspace(c)) {
        state = STATE_HEIGHT_SPACE;
      }
      else if (isdigit(c)) {
        str_height += c;
      }
      else {
        return false;
      }
      break;

      case STATE_HEIGHT_SPACE:
      if (c == '#') {
        skipUntilEol(input);
      }
      else if (isspace(c)) {
        // STAY HERE
      }
      else if (isdigit(c)) {
        str_depth += c;
        state = STATE_DEPTH;
      }
      else {
        return false;
      }
      break;

      case STATE_DEPTH:
      if (c == '#') {
        skipUntilEol(input);
        state = STATE_END;
      }
      else if (isspace(c)) {
        state = STATE_END;
      }
      else if (isdigit(c)) {
        str_depth += c;
      }
      else {
        return false;
      }
      break;

      case STATE_END:
      input.unget();

      width = stoi(str_width);
      if (width <= 0) {
        return false;
      }

      height = stoi(str_height);
      if (height <= 0) {
        return false;
      }

      depth = stoi(str_depth);
      if (depth > 65535 || depth <= 0) {
        return false;
      }

      return true;
      break;
    }
  }

  return false;
}
