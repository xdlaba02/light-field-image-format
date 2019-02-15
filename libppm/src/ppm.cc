/******************************************************************************\
* SOUBOR: ppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "ppm.h"

#include <string>

using namespace std;

enum PPMHeaderParserState {
  STATE_INIT,
  STATE_P,
  STATE_P6,
  STATE_P6_SPACE,
  STATE_WIDTH,
  STATE_WIDTH_SPACE,
  STATE_HEIGHT,
  STATE_HEIGHT_SPACE,
  STATE_DEPTH,
  STATE_END
};

Pixel *allocPPMRow(size_t width) {
  return new Pixel[width];
}
void freePPMRow(Pixel *row) {
  delete [] row;
}

void skipUntilEol(FILE *input) {
  int c {};
  while((c = getc(input)) != EOF) {
    if (c == '\n') {
      return;
    }
  }
  return;
}

int readPPMHeader(PPMFileStruct *ppm) {
  string str_width  {};
  string str_height {};
  string str_depth  {};

  PPMHeaderParserState state = STATE_INIT;

  int c {};
  while((c = getc(ppm->file)) != EOF) {
    switch (state) {
      case STATE_INIT:
      if (c == 'P') {
        state = STATE_P;
      }
      else {
        return -1;
      }
      break;

      case STATE_P:
      if (c == '6') {
        state = STATE_P6;
      }
      else {
        return -1;
      }
      break;

      case STATE_P6:
      if (c == '#') {
        skipUntilEol(ppm->file);
        state = STATE_P6_SPACE;
      }
      else if (isspace(c)) {
        state = STATE_P6_SPACE;
      }
      else {
        return -1;
      }
      break;

      case STATE_P6_SPACE:
      if (c == '#') {
        skipUntilEol(ppm->file);
      }
      else if (isspace(c)) {
        // STAY HERE
      }
      else if (isdigit(c)) {
        str_width += c;
        state = STATE_WIDTH;
      }
      else {
        return -1;
      }
      break;

      case STATE_WIDTH:
      if (c == '#') {
        skipUntilEol(ppm->file);
        state = STATE_WIDTH_SPACE;
      }
      else if (isspace(c)) {
        state = STATE_WIDTH_SPACE;
      }
      else if (isdigit(c)) {
        str_width += c;
      }
      else {
        return -1;
      }
      break;

      case STATE_WIDTH_SPACE:
      if (c == '#') {
        skipUntilEol(ppm->file);
      }
      else if (isspace(c)) {
        // STAY HERE
      }
      else if (isdigit(c)) {
        str_height += c;
        state = STATE_HEIGHT;
      }
      else {
        return -1;
      }
      break;

      case STATE_HEIGHT:
      if (c == '#') {
        skipUntilEol(ppm->file);
        state = STATE_HEIGHT_SPACE;
      }
      else if (isspace(c)) {
        state = STATE_HEIGHT_SPACE;
      }
      else if (isdigit(c)) {
        str_height += c;
      }
      else {
        return -1;
      }
      break;

      case STATE_HEIGHT_SPACE:
      if (c == '#') {
        skipUntilEol(ppm->file);
      }
      else if (isspace(c)) {
        // STAY HERE
      }
      else if (isdigit(c)) {
        str_depth += c;
        state = STATE_DEPTH;
      }
      else {
        return -1;
      }
      break;

      case STATE_DEPTH:
      if (c == '#') {
        skipUntilEol(ppm->file);
        state = STATE_END;
      }
      else if (isspace(c)) {
        state = STATE_END;
      }
      else if (isdigit(c)) {
        str_depth += c;
      }
      else {
        return -1;
      }
      break;

      case STATE_END:
      ungetc(c, ppm->file);

      int64_t width = stoi(str_width);
      if (width <= 0) {
        return -1;
      }

      int64_t height = stoi(str_height);
      if (height <= 0) {
        return -1;
      }

      int64_t depth = stoi(str_depth);
      if (depth > 65535 || depth <= 0) {
        return -1;
      }

      ppm->width = width;
      ppm->height = height;
      ppm->color_depth = depth;

      return 0;
      break;
    }
  }

  return -1;
}

int readPPMRow(PPMFileStruct *ppm, Pixel *buffer) {
  size_t units_read = 0;

  if (ppm->color_depth < 256) {
    for (size_t i = 0; i < ppm->width; i++) {
      uint8_t tmp {};

      units_read += fread(&tmp, 1, 1, ppm->file);
      buffer[i].r = tmp;

      units_read += fread(&tmp, 1, 1, ppm->file);
      buffer[i].g = tmp;

      units_read += fread(&tmp, 1, 1, ppm->file);
      buffer[i].b = tmp;
    }
  }
  else {
    for (size_t i = 0; i < ppm->width; i++) {
      uint16_t tmp {};

      units_read += fread(&tmp, 2, 1, ppm->file);
      buffer[i].r = be16toh(tmp);

      units_read += fread(&tmp, 2, 1, ppm->file);
      buffer[i].g = be16toh(tmp);

      units_read += fread(&tmp, 2, 1, ppm->file);
      buffer[i].b = be16toh(tmp);
    }
  }

  if (units_read != ppm->width * 3) {
    return -1;
  }

  return 0;
}

int writePPMHeader(const PPMFileStruct *ppm) {
  if (fprintf(ppm->file, "P6\n%lu\n%lu\n%u\n", ppm->width, ppm->height, ppm->color_depth) < 0) {
    return -1;
  }

  return 0;
}

int writePPMRow(PPMFileStruct *ppm, const Pixel *buffer) {
  size_t units_written = 0;

  if (ppm->color_depth < 256) {
    for (size_t i = 0; i < ppm->width; i++) {
      uint8_t tmp {};

      tmp = buffer[i].r;
      units_written += fwrite(&tmp, 1, 1, ppm->file);

      tmp = buffer[i].g;
      units_written += fwrite(&tmp, 1, 1, ppm->file);

      tmp = buffer[i].b;
      units_written += fwrite(&tmp, 1, 1, ppm->file);
    }
  }
  else {
    for (size_t i = 0; i < ppm->width; i++) {
      uint16_t tmp {};

      tmp = htobe16(buffer[i].r);
      units_written += fwrite(&tmp, 2, 1, ppm->file);

      tmp = htobe16(buffer[i].g);
      units_written += fwrite(&tmp, 2, 1, ppm->file);

      tmp = htobe16(buffer[i].b);
      units_written += fwrite(&tmp, 2, 1, ppm->file);
    }
  }

  if (units_written != ppm->width * 3) {
    return -1;
  }

  return 0;
}
