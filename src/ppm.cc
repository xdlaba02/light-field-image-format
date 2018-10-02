#include "ppm.h"
#include <vector>

bool loadPPM(const std::string &filename, BitmapRGB &rgb) {
  std::ifstream input(filename);
  if (input.fail()) {
    return false;
  }

  uint64_t width;
  uint64_t height;
  uint32_t depth;

  if (!parseHeader(input, width, height, depth)) {
    return false;
  }

  uint64_t data_size = width * height * 3;

  if (depth > 255) {
    data_size *= 2;
  }

  std::vector<uint8_t> tmp(data_size);

  input.read(reinterpret_cast<char *>(tmp.data()), data_size);
  if (input.fail()) {
    return false;
  }

  rgb.init(width, height);
  if (!rgb.initialized()) {
    return false;
  }

  for (uint64_t i = 0; i < rgb.sizeInBytes(); i++) {
    uint64_t byteValue;

    if (depth > 255) {
      byteValue = reinterpret_cast<uint16_t *>(tmp.data())[i] * 255 / depth;
    }
    else {
      byteValue = reinterpret_cast<uint8_t *>(tmp.data())[i] * 255 / depth;
    }

    rgb.data()[i] = byteValue;
  }

  return true;
}

bool savePPM(const std::string &filename, BitmapRGB &rgb) {
  std::ofstream output(filename);
  if (output.fail()) {
    return false;
  }

  output << "P6" << std::endl;
  output << rgb.width() << std::endl;
  output << rgb.height() << std::endl;
  output << "255" << std::endl;

  output.write(reinterpret_cast<char *>(rgb.data()), rgb.sizeInBytes());
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

  while(int c = input.get()) {
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
