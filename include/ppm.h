#ifndef PPM_H
#define PPM_H

#include <fstream>

class Bitmap;

enum State {
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

Bitmap *loadPPM(const char *filename);
bool savePPM(const char *filename, Bitmap *bitmap);

void skipUntilEol(std::ifstream &input);
bool parseHeader(std::ifstream &input, uint64_t &width, uint64_t &height, uint32_t &depth);
void writeHeader(std::ofstream &output, uint64_t width, uint64_t height, uint32_t depth);


#endif
