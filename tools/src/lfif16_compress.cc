/******************************************************************************\
* SOUBOR: lfif16_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <lfif16.h>
#include <ppm.h>

#include <getopt.h>

#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

void print_usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file> -o <output-file> -q <quality>" << endl;
}

bool parse_args(int argc, char *argv[], const char *&input_file_name, const char *&output_file_name, uint8_t &quality) {
  const char *arg_quality {};

  char opt;
  while ((opt = getopt(argc, argv, "i:o:q:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_name) {
          input_file_name = optarg;
          continue;
        }
        break;

      case 'o':
        if (!output_file_name) {
          output_file_name = optarg;
          continue;
        }
        break;

      case 'q':
        if (!arg_quality) {
          arg_quality = optarg;
          continue;
        }
        break;

      default:
        break;
    }

    print_usage(argv[0]);
    return false;
  }

  if ((!input_file_name) || (!output_file_name) || (!arg_quality)) {
    print_usage(argv[0]);
    return false;
  }

  int tmp_quality = atoi(arg_quality);
  if ((tmp_quality < 1) || (tmp_quality > 100)) {
    print_usage(argv[0]);
    return false;
  }

  quality = tmp_quality;

  return true;
}

int main(int argc, char *argv[]) {
  const char *input_file_name  {};
  const char *output_file_name {};
  uint8_t quality         {};

  if (!parse_args(argc, argv, input_file_name, output_file_name, quality)) {
    return 1;
  }

  uint64_t width       {};
  uint64_t height      {};
  uint32_t color_depth {};
  vector<uint8_t> rgb_data {};

  ifstream input(input_file_name);
  if (input.fail()) {
    cerr << "ERROR: CANNOT OPEN PPM" << endl;
    return 2;
  }

  if (readPPM(input, rgb_data, width, height, color_depth)) {
    cerr << "ERROR: BAD PPM" << endl;
    return 2;
  }

  if (color_depth != 65535) {
    cerr << "ERROR: UNSUPPORTED COLOR DEPTH. YET." << endl;
    return 2;
  }

  uint64_t img_dims[] {width, height};
  int errcode = LFIFCompress16(reinterpret_cast<uint16_t *>(rgb_data.data()), img_dims, quality, output_file_name);

  switch (errcode) {
    case -1:
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << output_file_name << "\" FOR WRITITNG" << endl;
      return 3;
    break;

    default:
      return 0;
    break;
  }

  return 0;
}
