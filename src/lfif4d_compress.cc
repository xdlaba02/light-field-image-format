/*******************************************************\
* SOUBOR: lfif4d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 20. 12. 2018
\*******************************************************/

#include "compress.h"

using namespace std;

int main(int argc, char *argv[]) {
  string input_file_mask  {};
  string output_file_name {};
  uint8_t quality         {};

  if (!parse_args(argc, argv, input_file_mask, output_file_name, quality)) {
    return -1;
  }

  uint64_t width  {};
  uint64_t height {};
  uint64_t depth  {};

  RGBData rgb_data {};

  if (!loadPPMs(input_file_mask, width, height, depth, rgb_data)) {
    return -2;
  }

  if (!compress<4>(output_file_name, width, height, depth, quality, rgb_data)) {
    return 3;
  }

  return 0;
}
