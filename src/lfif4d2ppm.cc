/*******************************************************\
* SOUBOR: lfif4d2ppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 22. 12. 2018
\*******************************************************/

#include "decompress.h"
#include "lfif_decoder.h"

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_file_name  {};
  const char *output_file_mask {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return 1;
  }

  uint64_t img_dims[4] {};
  uint64_t image_count {};

  RGBData rgb_data {};

  if (!LFIFDecompress<4>(input_file_name, rgb_data, img_dims, image_count)) {
    return 2;
  }

  if (!savePPMs(rgb_data, img_dims[0], img_dims[1], 255, img_dims[2] * img_dims[3] * image_count, output_file_mask)) {
    return 3;
  }

  return 0;
}
