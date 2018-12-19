#include "compress.h"

#include <iostream>

void print_usage(const char *argv0) {
  std::cerr << "Usage: " << std::endl;
  std::cerr << argv0 << " -i <file> -o <file-mask>" << std::endl;
}
