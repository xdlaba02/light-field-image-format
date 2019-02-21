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

  LFIFCompressStruct cinfo {};

  if (!parse_args(argc, argv, input_file_mask, output_file_name, quality)) {
    return 1;
  }

  if (!checkPPMheaders(input_file_mask, width, height, color_depth, image_count)) {
    return 2;
  }

  if (color_depth < 256) {
    rgb_data.resize(width * height * image_count * 3);
  }
  else {
    rgb_data.resize(width * height * image_count * 3 * 2);
  }

  if (!loadPPMs(input_file_mask, rgb_data.data())) {
    return 2;
  }

  cinfo.image_width      = width;
  cinfo.image_height     = height;
  cinfo.image_count      = image_count;
  cinfo.quality          = quality;
  cinfo.method           = LFIF_2D;
  cinfo.output_file_name = output_file_name;

  switch (color_depth) {
    case 255:
      cinfo.color_space = RGB24;
    break;

    case 65535:
      cinfo.color_space = RGB48;
    break;

    default:
      cerr << "ERROR: UNSUPPORTED COLOR DEPTH" << endl;
      return 3;
    break;
  }

  int errcode = LFIFCompress(&cinfo, rgb_data.data());

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
