/*******************************************************\
* SOUBOR: main.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "jpeg2d.h"

#include <iostream>

using namespace std;

void print_usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " --encode <1..100> path/to/input.ppm path/to/output.jpeg2d" << endl;
  cerr << argv0 << " --decode path/to/input.jpeg2d path/to/output.ppm" << endl;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    return -1;
  }

  string type {argv[1]};

  if (type == "-e" || type == "--encode") {
    if (argc != 5) {
      print_usage(argv[0]);
      return -1;
    }

    int8_t quality {static_cast<int8_t>(atoi(argv[2]))};
    if ((quality < 1) || (quality > 100)) {
      print_usage(argv[0]);
      return -1;
    }

    if (!PPM2JPEG2D(argv[3], argv[4], quality)) {
      return -2;
    }
  }
  else if (type == "-d" || type == "--decode") {
    if (argc != 4) {
      print_usage(argv[0]);
      return -1;
    }
    if (!JPEG2D2PPM(argv[2], argv[3])) {
      return -2;
    }
  }
  else {
    print_usage(argv[0]);
    return -1;
  }

  return 0;
}
