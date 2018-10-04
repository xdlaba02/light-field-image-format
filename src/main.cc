#include "ppm.h"
#include "bitmap.h"
#include "codec.h"
#include "jpeg2d.h"
#include "bitstream.h"

#include <iostream>

void print_usage(const char *argv0) {
  std::cout << "Usage: " << std::endl;
  std::cout << argv0 << " --encode <quality> path/to/file.ppm" << std::endl;
  std::cout << argv0 << " --decode path/to/file.jpeg2d" << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    print_usage(argv[0]);
    return 1;
  }

  std::string type(argv[1]);

  BitmapRGB rgb;
  JPEG2D jpeg;

  if (type == "-e" || type == "--encode") {
    std::string quality(argv[2]);
    std::string filename(argv[3]);

    loadPPM(filename, rgb);
    jpegEncode(rgb, stoi(quality), jpeg);
    saveJPEG(filename+".jpeg2d", jpeg);
  }
  else if (type == "-d" || type == "--decode") {
    std::string filename(argv[2]);

    //loadJPEG(filename, jpeg);
    //jpegDecode(jpeg, rgb);
    //savePPM(filename+".ppm", rgb);
  }
  else {
    print_usage(argv[0]);
    return 1;
  }

  return 0;
}
