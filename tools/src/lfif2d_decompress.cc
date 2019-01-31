/******************************************************************************\
* SOUBOR: lfif2d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "decompress.h"
#include "lfifppm.h"

#include <lfif_decoder.h>

#include <iostream>

int main(int argc, char *argv[]) {
  const char *input_file_name  {};
  const char *output_file_mask {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return 1;
  }

  uint64_t img_dims[2] {};
  uint64_t image_count {};

  RGBData rgb_data {};

  int errcode = LFIFDecompress<2>(input_file_name, rgb_data, img_dims, image_count);

  switch (errcode) {
    case -1:
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << input_file_name << "\" FOR READING" << endl;
      return 2;
    break;

    case -2:
      cerr << "ERROR: MAGIC NUMBER MISMATCH" << endl;
      return 2;
    break;

    default:
    break;
  }

  if (!savePPMs(rgb_data, img_dims[0], img_dims[1], 255, image_count, output_file_mask)) {
    return 3;
  }

  return 0;
}
