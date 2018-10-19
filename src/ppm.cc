/*******************************************************\
* SOUBOR: ppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "ppm.h"
#include <vector>
#include <iostream>

bool loadPPM(const char *filename, uint64_t &width, uint64_t &height, vector<uint8_t> &data) {
  ifstream input(filename);
  if (input.fail()) {
    return false;
  }

  uint32_t depth {};

  if (!parseHeader(input, width, height, depth)) {
    return false;
  }

  if (depth != 255) {
    return false;
  }
  else {
    data.resize(width * height * 3);
    input.read(reinterpret_cast<char *>(data.data()), data.size());
  }

  if (input.fail()) {
    return false;
  }

  return true;
}

bool savePPM(const char *filename, const uint64_t width, const uint64_t height, vector<uint8_t> &data) {
  ofstream output(filename);
  if (output.fail()) {
    return false;
  }

  output << "P6" << endl;
  output << width << endl;
  output << height << endl;
  output << "255" << endl;

  output.write(reinterpret_cast<char *>(data.data()), data.size());
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
