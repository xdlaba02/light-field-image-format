#include "ppm.h"
#include "jpeg2d_encoder.h"
#include "jpeg2d_decoder.h"

#include <iostream>

void print_usage(const char *argv0) {
  std::cout << "Usage: " << std::endl;
  std::cout << argv0 << " --encode <1..100> path/to/file.ppm" << std::endl;
  std::cout << argv0 << " --decode path/to/file.jpeg2d" << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    print_usage(argv[0]);
    return 1;
  }

  std::string type(argv[1]);

  std::vector<uint8_t> rgb;
  uint64_t width;
  uint64_t height;

  if (type == "-e" || type == "--encode") {
    uint8_t quality = stoi(std::string(argv[2]));
    if ((quality < 1) || (quality > 100)) {
      print_usage(argv[0]);
      return 1;
    }

    std::string filename(argv[3]);

    if (!loadPPM(filename, width, height, rgb)) {
      std::cout << "Unable to open file \"" << filename << "\"" << std::endl;
      return 1;
    }

    JPEG2DEncoder encoder(width, height, rgb.data(), quality);

    encoder.run();
    encoder.save(filename+".jpeg2d");
  }
  else if (type == "-d" || type == "--decode") {
    std::string filename(argv[2]);

    JPEG2DDecoder decoder;
    if (!decoder.load(filename)) {
      std::cout << "Unable to open file \"" << filename << "\"" << std::endl;
      return 1;
    }

    decoder.run(width, height, rgb);

    savePPM(filename+".ppm", width, height, rgb);

  }
  else {
    print_usage(argv[0]);
    return 1;
  }

  return 0;
}
