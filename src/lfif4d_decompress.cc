/*******************************************************\
* SOUBOR: lfif4d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 20. 12. 2018
\*******************************************************/

#include "decompress.h"

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_file_name  {};
  const char *output_file_mask {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return 1;
  }

  uint64_t width       {};
  uint64_t height      {};
  uint64_t image_count {};

  RGBData rgb_data {};

  if (!decompress<4>(input_file_name, rgb_data, width, height, image_count)) {
    return 2;
  }

  if (!savePPMs(rgb_data, width, height, 255, image_count, output_file_mask)) {
    return 3;
  }

  return 0;
}
