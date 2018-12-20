/*******************************************************\
* SOUBOR: lfif4d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 20. 12. 2018
\*******************************************************/

#include "decompress.h"

using namespace std;

int main(int argc, char *argv[]) {
  string input_file_name  {};
  string output_file_mask {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return 1;
  }

  uint64_t width  {};
  uint64_t height {};
  uint64_t depth  {};

  RGBData rgb_data {};

  if (!decompress<4>(input_file_name, width, height, depth, rgb_data)) {
    return 2;
  }

  if (!savePPMs(output_file_mask, width, height, depth, rgb_data)) {
    return 3;
  }

  return 0;
}
