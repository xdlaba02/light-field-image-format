/******************************************************************************\
* SOUBOR: ppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "ppm.h"

#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>

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

int PPM::parseHeader(FILE *ppm) {
  string str_width  {};
  string str_height {};
  string str_depth  {};

  PPMHeaderParserState state = STATE_INIT;

  int c {};
  while((c = getc(ppm)) != EOF) {
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
        skipUntilEol(ppm);
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
        skipUntilEol(ppm);
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
        skipUntilEol(ppm);
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
        skipUntilEol(ppm);
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
        skipUntilEol(ppm);
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
        skipUntilEol(ppm);
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
        skipUntilEol(ppm);
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
      ungetc(c, ppm);

      int64_t signed_width = stoi(str_width);
      if (signed_width <= 0) {
        return -1;
      }

      int64_t signed_height = stoi(str_height);
      if (signed_height <= 0) {
        return -1;
      }

      int64_t signed_depth = stoi(str_depth);
      if (signed_depth > 65535 || signed_depth <= 0) {
        return -1;
      }

      m_width       = signed_width;
      m_height      = signed_height;
      m_color_depth = signed_depth;

      return 0;
      break;
    }
  }

  return -1;
}

int PPM::mmapPPM(const char *file_name) {
  FILE *ppm = fopen(file_name, "rb");
  if (!ppm) {
    return -1;
  }

  if (parseHeader(ppm) < 0) {
    fclose(ppm);
    return -2;
  }

  m_header_offset = ftell(ppm);
  fclose(ppm);

  size_t file_size = m_width * m_height * (m_color_depth > 255 ? 2 : 1) * 3;

  int fd = open(file_name, O_RDONLY);
  if (fd < 0) {
    return -3;
  }

  m_file = mmap(NULL, file_size + m_header_offset, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  close(fd);

  if (m_file == MAP_FAILED) {
    return -4;
  }

  m_opened = true;

  return 0;
}

PPM::PPM(PPM &&other): m_opened{other.m_opened}, m_width{other.m_width}, m_height{other.m_height}, m_color_depth{other.m_color_depth}, m_file{other.m_file}, m_header_offset{other.m_header_offset} {
  other.m_opened        = false;
  other.m_width         = 0;
  other.m_height        = 0;
  other.m_color_depth   = 0;
  other.m_file          = nullptr;
  other.m_header_offset = 0;
}

#include <iostream>

PPM::~PPM() {
  if (m_opened) {
    size_t file_size = m_width * m_height * (m_color_depth > 255 ? 2 : 1);
    if (munmap(m_file, file_size) < 0) {
      std::cerr << "munmap fail\n";
    }
  }
}

int PPM::createPPM(const char *file_name, uint64_t width, uint64_t height, uint32_t color_depth) {
  FILE *ppm = fopen(file_name, "wb");
  if (!ppm) {
    return -1;
  }

  if (fprintf(ppm, "P6\n%lu\n%lu\n%u\n", width, height, color_depth) < 0) {
    fclose(ppm);
    return -1;
  }

  m_header_offset = ftell(ppm);
  fclose(ppm);

  size_t file_size = width * height * (color_depth > 255 ? 2 : 1) * 3;

  int fd = open(file_name, O_RDWR);
  if (fd < 0) {
    return -3;
  }

  if (ftruncate(fd, file_size + m_header_offset) < 0) {
    return -4;
  }

  m_file = mmap(NULL, file_size + m_header_offset, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  if (m_file == MAP_FAILED) {
    return -5;
  }

  m_opened      = true;
  m_width       = width;
  m_height      = height;
  m_color_depth = color_depth;

  return 0;
}
