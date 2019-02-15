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

  PPMFileStruct ppm            {};
  Pixel *ppm_row               {};

  uint64_t img_dims[2] {};
  vector<uint16_t> rgb_data    {};

  if (!parse_args(argc, argv, input_file_name, output_file_name)) {
    return 1;
  }

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

  ppm.width = img_dims[0];
  ppm.height = img_dims[1];
  ppm.color_depth = 65535;

  ppm_row = allocPPMRow(ppm.width);

  size_t last_slash_pos = string(output_file_name).find_last_of('/');
  if (last_slash_pos != string::npos) {
    string command("mkdir -p " + string(output_file_name).substr(0, last_slash_pos));
    system(command.c_str());
  }

  ppm.file = fopen(output_file_name, "wb");
  if (!ppm.file) {
    cerr << "ERROR: CANNOT OPEN " << output_file_name << "FOR WRITING" << endl;
    return 3;
  }

  if (writePPMHeader(&ppm)) {
    cerr << "ERROR: CANNOT WRITE TO " << output_file_name << endl;
    return 3;
  }

  for (size_t row = 0; row < ppm.height; row++) {
    for (size_t col = 0; col < ppm.width; col++) {
      ppm_row[col].r = rgb_data[(row * ppm.width + col) * 3 + 0];
      ppm_row[col].g = rgb_data[(row * ppm.width + col) * 3 + 1];
      ppm_row[col].b = rgb_data[(row * ppm.width + col) * 3 + 2];
    }

    if (writePPMRow(&ppm, ppm_row)) {
      cerr << "ERROR: CANNOT WRITE TO " << output_file_name << endl;
      return 3;
    }
  }

  freePPMRow(ppm_row);
  fclose(ppm.file);
  return 0;
}
