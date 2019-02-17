/******************************************************************************\
* SOUBOR: lfif2d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"
#include "plenoppm.h"

#include <lfiflib.h>

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_file_mask  {};
  const char *output_file_name {};
  uint8_t quality              {};

  vector<uint8_t> rgb_data     {};

  uint64_t width               {};
  uint64_t height              {};
  uint32_t color_depth         {};
  uint64_t image_count         {};

  LFIFCompressStruct lfif_compress_struct {};

  if (!parse_args(argc, argv, input_file_mask, output_file_name, quality)) {
    return 1;
  }

  if (!checkPPMheaders(input_file_mask, width, height, color_depth, image_count)) {
    return 2;
  }

  rgb_data.resize(width * height * image_count * 3);

  if (!loadPPMs(input_file_mask, rgb_data.data())) {
    return 3;
  }

  lfif_compress_struct.image_width  = width;
  lfif_compress_struct.image_height = height;
  lfif_compress_struct.image_count  = image_count;
  lfif_compress_struct.quality      = quality;
  lfif_compress_struct.method       = LFIF_2D;
  lfif_compress_struct.color_space  = RGB24;
  lfif_compress_struct.rgb_data_24  = rgb_data.data();

  int errcode = LFIFCompress(&lfif_compress_struct, output_file_name);

  switch (errcode) {
    case -1:
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << output_file_name << "\" FOR WRITITNG" << endl;
      return 4;
    break;

    default:
      return 0;
    break;
  }

  return 0;
}
