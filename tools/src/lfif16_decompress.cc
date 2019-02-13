/******************************************************************************\
* SOUBOR: lfif16_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <lfif16.h>
#include <ppm.h>

#include <getopt.h>

#include <iostream>

void print_usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file> -o <output-file>" << endl;
}

bool parse_args(int argc, char *argv[], const char *&input_file_name, const char *&output_file_name) {
  char opt;
  while ((opt = getopt(argc, argv, "i:o:")) >= 0) {
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

      default:
        break;
    }

    print_usage(argv[0]);
    return false;
  }

  if ((!input_file_name) || (!output_file_name)) {
    print_usage(argv[0]);
    return false;
  }

  return true;
}

int main(int argc, char *argv[]) {
  const char *input_file_name  {};
  const char *output_file_name {};

  if (!parse_args(argc, argv, input_file_name, output_file_name)) {
    return 1;
  }

  uint64_t img_dims[2] {};
  vector<uint16_t> rgb_data {};

  int errcode = LFIFDecompress16(input_file_name, rgb_data, img_dims);

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

  ofstream output(output_file_name);
  if (output.fail()) {
    cerr << "ERROR: UNABLE TO OPEN FILE \"" << output_file_name << "\" FOR WRITING" << endl;
    return 3;
  }

  if (writePPM(reinterpret_cast<uint8_t *>(rgb_data.data()), img_dims[0], img_dims[1], 65535, output)) {
    return 3;
  }

  return 0;
}
