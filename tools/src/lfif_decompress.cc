/******************************************************************************\
* SOUBOR: lfif_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "decompress.h"
#include "plenoppm.h"

#include <lfiflib.h>

#include <cmath>

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
        int   errcode          {};
  const char *input_file_name  {};
  const char *output_file_mask {};

  vector<uint8_t> rgb_data     {};

  LFIFDecompressStruct dinfo   {};

  if (!parse_args(argc, argv, input_file_name, output_file_mask)) {
    return 1;
  }

  dinfo.input_file_name = input_file_name;

  errcode = LFIFReadHeader(&dinfo);

  if (dinfo.max_rgb_value < 256) {
    rgb_data.resize(dinfo.image_width * dinfo.image_height * dinfo.image_count * 3);

  }
  else {
    rgb_data.resize(dinfo.image_width * dinfo.image_height * dinfo.image_count * 3 * 2);
  }

  errcode = LFIFDecompress(&dinfo, rgb_data.data());

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

  if (!savePPMs(output_file_mask, rgb_data.data(), dinfo.image_width, dinfo.image_height, dinfo.max_rgb_value, dinfo.image_count)) {
    return 3;
  }

  return 0;
}
