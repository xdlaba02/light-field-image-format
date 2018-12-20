/*******************************************************\
* SOUBOR: lfif2d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 12. 2018
\*******************************************************/

#include "decompress.h"

using namespace std;

int main(int argc, char *argv[]) {
  string input_file_name  {};
  string output_file_mask {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return 1;
  }

  uint64_t width       {};
  uint64_t height      {};
  uint32_t color_depth {255};
  uint64_t image_count {};

  RGBData rgb_data {};

  if (!decompress<2>(input_file_name, rgb_data, width, height, color_depth, image_count)) {
    return 2;
  }

  if (!savePPMs(rgb_data, width, height, color_depth, image_count, output_file_mask)) {
    return 3;
  }

  return 0;
}
