#include "ppm.h"
#include "bitmap.h"
#include "image_jpeg.h"
#include "codec.h"

#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Usage: " << std::endl << argv[0] << " -e <quality> path/to/file.ppm" << std::endl;
    return 1;
  }

  std::string type(argv[1]);

  if (type == "-e" || type == "--encode") {
    std::string quality(argv[2]);
    std::string filename(argv[3]);

    BitmapRGB rgb;
    loadPPM(filename, rgb);

    ImageJPEG jpeg;
    jpegEncode(rgb, stoi(quality), jpeg);

    //saveJPEG("potato.jpeg2d", jpeg);
  }
  else if (type == "-d" || type == "--decode") {
    std::string filename(argv[2]);

    ImageJPEG jpeg;
    //loadJPEG(filename, jpeg);

    BitmapRGB rgb;
    //jpegDecode(jpeg, rgb);

    savePPM(std::string("potato.ppm"), rgb);
  }
  else {
    std::cout << "Usage: " << std::endl << argv[0] << " -e <quality> path/to/file.ppm" << std::endl;
    return 1;
  }

  return 0;
}
