#include "ppm.h"
#include "bitmap.h"
#include "codec.h"

#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << std::endl << argv[0] << " path/to/file.ppm" << std::endl;
    return 1;
  }

  Bitmap *image = loadPPM(argv[1]);
  delete image;

  return 0;
}
