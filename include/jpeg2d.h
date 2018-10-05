#ifndef JPEG2D_H
#define JPEG2D_H

#include "huffman.h"
#include "bitstream.h"

#include <cstdint>



class JPEG2D {
public:
  JPEG2D();
  bool load(const std::string filename);

private:
};

#endif
