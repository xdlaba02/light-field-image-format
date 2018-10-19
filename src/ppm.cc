/*******************************************************\
* SOUBOR: ppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "ppm.h"
#include <vector>
#include <iostream>

bool loadPPM(const char *filename, uint64_t &width, uint64_t &height, std::vector<uint8_t> &data) {
  std::ifstream input(filename);
  if (input.fail()) {
    return false;
  }

  uint32_t depth;

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

bool savePPM(const char *filename, const uint64_t width, const uint64_t height, std::vector<uint8_t> &data) {
  std::ofstream output(filename);
  if (output.fail()) {
    return false;
  }

  output << "P6" << std::endl;
  output << width << std::endl;
  output << height << std::endl;
  output << "255" << std::endl;

  output.write(reinterpret_cast<char *>(data.data()), data.size());
  if (output.fail()) {
    return false;
  }

  return true;
}

void skipUntilEol(std::ifstream &input) {
  while(int c = input.get()) {
    if (input.eof() || c == '\n') {
      return;
    }
  }
  return;
}

bool parseHeader(std::ifstream &input, uint64_t &width, uint64_t &height, uint32_t &depth) {
  std::string str_width;
  std::string str_height;
  std::string str_depth;

  State state = STATE_INIT;

  uint8_t c = 0;
  while(input.read(reinterpret_cast<char *>(&c), 1)) {
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
      else if (std::isdigit(c)) {
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
      else if (std::isdigit(c)) {
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
      else if (std::isdigit(c)) {
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
      else if (std::isdigit(c)) {
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
      else if (std::isdigit(c)) {
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
      else if (std::isdigit(c)) {
        str_depth += c;
      }
      else {
        return false;
      }
      break;

      case STATE_END:
      input.unget();

      width = std::stoi(str_width);
      if (width <= 0) {
        return false;
      }

      height = std::stoi(str_height);
      if (height <= 0) {
        return false;
      }

      depth = std::stoi(str_depth);
      if (depth > 65535 || depth <= 0) {
        return false;
      }

      return true;
      break;
    }
  }

  return false;
}
