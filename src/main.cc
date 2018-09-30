#include "ppm.h"
#include "codec.h"

#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << std::endl << argv[0] << " path/to/file.ppm" << std::endl;
    return 1;
  }

  uint64_t width;
  uint64_t height;

  uint8_t *image = loadPPM(argv[1], width, height);
  uint8_t *compressedImage = encodeRGB(width, height, image);

  delete compressedImage;
  delete image;

  return 0;
}
