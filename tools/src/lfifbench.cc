/******************************************************************************\
* SOUBOR: lfifbench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <lfiflib.h>

#include <getopt.h>

#include <cmath>

#include <thread>
#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> [-2 <output-file-name>] [-3 <output-file-name>] [-4 <output-file-name>] [-s <quality-step>]" << endl;
}

double PSNR(const vector<uint8_t> &original, const vector<uint8_t> &compared) {
  double mse  {};

  if (original.size() != compared.size()) {
    return 0.0;
  }

  for (size_t i = 0; i < original.size(); i++) {
    mse += (original[i] - compared[i]) * (original[i] - compared[i]);
  }

  mse /= original.size();

  if (!mse) {
    return 0;
  }

  return 10 * log10((255.0 * 255.0) / mse);
}

size_t encode(LFIFCompressStruct &cinfo, const char *filename) {
  int errcode = LFIFCompress(&cinfo, filename);

  if (errcode) {
    std::cerr << "ERROR: UNABLE TO OPEN FILE \"" << filename << "\" FOR WRITITNG" << endl;
    return 0;
  }

  ifstream encoded_file(filename, ifstream::ate | ifstream::binary);
  return encoded_file.tellg();
}

size_t fileSize(const char *filename) {
  ifstream encoded_file(filename, ifstream::ate | ifstream::binary);
  return encoded_file.tellg();
}

int doTest(LFIFCompressStruct cinfo, LFIFDecompressStruct dinfo, const vector<uint8_t> &original, ofstream &output, size_t q_step) {
  size_t image_pixels          {};
  size_t compressed_image_size {};
  int    errcode               {};
  double psnr                  {};
  double bpp                   {};
  vector<uint8_t> decompressed {};

  image_pixels = cinfo.image_width * cinfo.image_width * cinfo.image_count;

  decompressed.resize(image_pixels * 3);

  for (size_t quality = q_step; quality <= 100; quality += q_step) {
    cinfo.quality = quality;
    errcode = LFIFCompress(&cinfo, original.data());
    compressed_image_size = fileSize(cinfo.output_file_name);
    errcode = LFIFDecompress(&dinfo, decompressed.data());
    psnr = PSNR(original, decompressed);
    bpp = compressed_image_size * 8.0 / image_pixels;
    output << quality  << " " << psnr << " " << bpp << endl;
  }

  return 0;
}

int main(int argc, char *argv[]) {
  const char *input_file_mask {};
  const char *output_file_2D  {};
  const char *output_file_3D  {};
  const char *output_file_4D  {};
  const char *quality_step    {};
  bool nothreads              {};
  uint8_t q_step              {};

  vector<uint8_t> rgb_data    {};

  uint64_t width       {};
  uint64_t height      {};
  uint32_t color_depth {};
  uint64_t image_count {};

  LFIFCompressStruct   cinfo {};
  LFIFDecompressStruct dinfo {};

  vector<thread>       threads   {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:s:2:3:4:n")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
          continue;
        }
      break;

      case 's':
        if (!quality_step) {
          quality_step = optarg;
          continue;
        }
      break;

      case '2':
        if (!output_file_2D) {
          output_file_2D = optarg;;
          continue;
        }
      break;

      case '3':
        if (!output_file_3D) {
          output_file_3D = optarg;;
          continue;
        }
      break;

      case '4':
        if (!output_file_4D) {
          output_file_4D = optarg;;
          continue;
        }
      break;

      case 'n':
        if (!nothreads) {
          nothreads = true;;
          continue;
        }
      break;

      default:
        print_usage(argv[0]);
        return 1;
      break;
    }
  }

  if (!input_file_mask) {
    print_usage(argv[0]);
    return 1;
  }

  if (!output_file_2D && !output_file_3D && !output_file_4D) {
    cerr << "Please specify one or more options [-2 <output-filename>] [-3 <output-filename>] [-4 <output-filename>]." << endl;
    print_usage(argv[0]);
    return 1;
  }

  q_step = 1;
  if (quality_step) {
    int tmp = atoi(quality_step);
    if ((tmp < 1) || (tmp > 100)) {
      print_usage(argv[0]);
      return 1;
    }
    q_step = tmp;
  }

  if (!checkPPMheaders(input_file_mask, width, height, color_depth, image_count)) {
    return 2;
  }

  rgb_data.resize(width * height * image_count * 3);

  if (!loadPPMs(input_file_mask, rgb_data.data())) {
    return 3;
  }

  if (color_depth != 255) {
    cerr << "ERROR: UNSUPPORTED COLOR DEPTH. YET." << endl;
    return 2;
  }

  cinfo.image_width      = width;
  cinfo.image_height     = height;
  cinfo.image_count      = image_count;
  cinfo.color_space      = RGB24;

  if (output_file_2D) {
    cinfo.method = LFIF_2D;
    cinfo.output_file_name = "/tmp/lfifbench.lfi2d";
    dinfo.input_file_name = "/tmp/lfifbench.lfi2d";

    ofstream output(output_file_2D);
    output << "'2D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;

    if (nothreads) {
      doTest(cinfo, dinfo, rgb_data, output, q_step);
    }
    else {
      threads.push_back(thread(doTest, cinfo, dinfo, ref(rgb_data), ref(output), q_step));
    }
  }

  if (output_file_3D) {
    cinfo.method = LFIF_3D;
    cinfo.output_file_name = "/tmp/lfifbench.lfi3d";
    dinfo.input_file_name = "/tmp/lfifbench.lfi3d";

    ofstream output(output_file_3D);
    output << "'3D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;

    if (nothreads) {
      doTest(cinfo, dinfo, rgb_data, output, q_step);
    }
    else {
      threads.push_back(thread(doTest, cinfo, dinfo, ref(rgb_data), ref(output), q_step));
    }
  }

  if (output_file_4D) {
    cinfo.method = LFIF_4D;
    cinfo.output_file_name = "/tmp/lfifbench.lfi4d";
    dinfo.input_file_name = "/tmp/lfifbench.lfi4d";

    ofstream output(output_file_4D);
    output << "'4D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;

    if (nothreads) {
      doTest(cinfo, dinfo, rgb_data, output, q_step);
    }
    else {
      threads.push_back(thread(doTest, cinfo, dinfo, ref(rgb_data), ref(output), q_step));
    }
  }

  for (auto &thread: threads) {
    thread.join();
  }

  return 0;
}
