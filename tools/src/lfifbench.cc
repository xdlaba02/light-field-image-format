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

template<typename T>
double MSE(const T *cmp1, const T *cmp2, size_t size) {
  double mse = 0;
  for (size_t i = 0; i < size; i++) {
    mse += (cmp1[i] - cmp2[i]) * (cmp1[i] - cmp2[i]);
  }
  return mse / size;
}

double PSNR(double mse, size_t max) {
  if (mse == .0) {
    return 0;
  }
  return 10 * log10((max * max) / mse);
}

size_t fileSize(const char *filename) {
  ifstream encoded_file(filename, ifstream::ate | ifstream::binary);
  return encoded_file.tellg();
}

int doTest(LFIFCompressStruct cinfo, const vector<uint8_t> &original, ofstream &output, size_t q_step) {
  LFIFDecompressStruct dinfo   {};
  size_t image_pixels          {};
  size_t compressed_image_size {};
  int    errcode               {};
  double psnr                  {};
  double bpp                   {};
  vector<uint8_t> decompressed {};

  dinfo.image_width     = cinfo.image_width;
  dinfo.image_height    = cinfo.image_height;
  dinfo.image_count     = cinfo.image_count;
  dinfo.color_space     = cinfo.color_space;
  dinfo.method          = cinfo.method;
  dinfo.input_file_name = cinfo.output_file_name;

  image_pixels = cinfo.image_width * cinfo.image_height * cinfo.image_count;

  decompressed.resize(original.size());

  for (size_t quality = q_step; quality <= 100; quality += q_step) {
    cinfo.quality = quality;
    errcode = LFIFCompress(&cinfo, original.data());
    compressed_image_size = fileSize(cinfo.output_file_name);
    errcode = LFIFDecompress(&dinfo, decompressed.data());

    if (dinfo.color_space == RGB24) {
      psnr = PSNR(MSE<uint8_t>(original.data(), decompressed.data(), image_pixels * 3), 255);
    }
    else {
      psnr = PSNR(MSE<uint16_t>(reinterpret_cast<const uint16_t *>(original.data()), reinterpret_cast<const uint16_t *>(decompressed.data()), image_pixels * 3), 65535);
    }

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

  ofstream outputs[3] {};

  vector<thread> threads {};

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

  if (color_depth < 256) {
    rgb_data.resize(width * height * image_count * 3);
  }
  else {
    rgb_data.resize(width * height * image_count * 3 * 2);
  }

  if (!loadPPMs(input_file_mask, rgb_data.data())) {
    return 3;
  }

  cinfo.image_width  = width;
  cinfo.image_height = height;
  cinfo.image_count  = image_count;

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

  if (output_file_2D) {
    cinfo.method = LFIF_2D;
    cinfo.output_file_name = "/tmp/lfifbench.lfif2d";

    outputs[0].open(output_file_2D);
    outputs[0] << "'2D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;

    if (nothreads) {
      doTest(cinfo, rgb_data, outputs[0], q_step);
    }
    else {
      threads.push_back(thread(doTest, cinfo, ref(rgb_data), ref(outputs[0]), q_step));
    }
  }

  if (output_file_3D) {
    cinfo.method = LFIF_3D;
    cinfo.output_file_name = "/tmp/lfifbench.lfif3d";

    outputs[1].open(output_file_3D);
    outputs[1] << "'3D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;

    if (nothreads) {
      doTest(cinfo, rgb_data, outputs[1], q_step);
    }
    else {
      threads.push_back(thread(doTest, cinfo, ref(rgb_data), ref(outputs[1]), q_step));
    }
  }

  if (output_file_4D) {
    cinfo.method = LFIF_4D;
    cinfo.output_file_name = "/tmp/lfifbench.lfif4d";

    outputs[2].open(output_file_4D);

    outputs[2] << "'4D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;

    if (nothreads) {
      doTest(cinfo, rgb_data, outputs[2], q_step);
    }
    else {
      threads.push_back(thread(doTest, cinfo, ref(rgb_data), ref(outputs[2]), q_step));
    }
  }

  for (auto &thread: threads) {
    thread.join();
  }

  return 0;
}
