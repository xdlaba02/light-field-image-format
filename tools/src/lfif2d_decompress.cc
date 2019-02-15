/******************************************************************************\
* SOUBOR: lfif2d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "decompress.h"
#include "plenoppm.h"

#include <lfif_decoder.h>

#include <iostream>

int main(int argc, char *argv[]) {
  const char *input_file_name  {};
  const char *output_file_mask {};

  vector<uint8_t> rgb_data     {};
  uint64_t img_dims[2]         {};
  uint64_t image_count         {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return 1;
  }

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

  if (!savePPMs(output_file_mask, rgb_data.data(), img_dims[0], img_dims[1], 255, image_count)) {
    return 3;
  }

  return 0;
}
