/******************************************************************************\
* SOUBOR: lfifbench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <colorspace.h>
#include <lfif_encoder.h>
#include <lfif_decoder.h>

#include <getopt.h>

#include <cmath>

#include <thread>
#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

const size_t BLOCK_SIZE = 8;

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> [-2 <output-file-name>] [-3 <output-file-name>] [-4 <output-file-name>] [-f <fist-quality>] [-l <last-quality>] [-s <quality-step>] [-a]" << endl;
}

double PSNR(double mse, size_t max) {
  if (mse == .0) {
    return 0;
  }
  return 10 * log10((max * max) / mse);
}

template <size_t BS, size_t D>
int doTest(LfifEncoder<BS, D> encoder, const vector<uint8_t> &original, const array<float, 3> &quality_interval, ostream &data_output, const char *tmp_filename) {
  LfifDecoder<BS, D> decoder   {};

  size_t image_pixels          {};
  size_t compressed_image_size {};
  double mse                   {};

  ofstream output {};
  ifstream input  {};

  image_pixels = original.size() / ((encoder.max_rgb_value > 255) ? 6 : 3);

  auto inputF0 = [&](size_t channel, size_t index) -> RGBUNIT {
    if (encoder.max_rgb_value > 255) {
      return reinterpret_cast<const uint16_t *>(original.data())[index * 3 + channel];
    }
    else {
      return reinterpret_cast<const uint8_t *>(original.data())[index * 3 + channel];
    }
  };

  auto inputF = [&](size_t index) -> INPUTTRIPLET {
    RGBUNIT R = inputF0(0, index);
    RGBUNIT G = inputF0(1, index);
    RGBUNIT B = inputF0(2, index);

    INPUTUNIT  Y = YCbCr::RGBToY(R, G, B) - ((encoder.max_rgb_value + 1) / 2);
    INPUTUNIT Cb = YCbCr::RGBToCb(R, G, B);
    INPUTUNIT Cr = YCbCr::RGBToCr(R, G, B);

    return {Y, Cb, Cr};
  };

  auto outputF = [&](size_t index, const INPUTTRIPLET &triplet) {
    INPUTUNIT  Y = triplet[0] + ((decoder.max_rgb_value + 1) / 2);
    INPUTUNIT Cb = triplet[1];
    INPUTUNIT Cr = triplet[2];

    RGBUNIT R = clamp<INPUTUNIT>(round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, decoder.max_rgb_value);
    RGBUNIT G = clamp<INPUTUNIT>(round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, decoder.max_rgb_value);
    RGBUNIT B = clamp<INPUTUNIT>(round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, decoder.max_rgb_value);

    mse += (inputF0(0, index) - R) * (inputF0(0, index) - R);
    mse += (inputF0(1, index) - G) * (inputF0(1, index) - G);
    mse += (inputF0(2, index) - B) * (inputF0(2, index) - B);
  };

  size_t last_slash_pos = string(tmp_filename).find_last_of('/');
  if (last_slash_pos != string::npos) {
    string command = "mkdir -p " + string(tmp_filename).substr(0, last_slash_pos);
    system(command.c_str());
  }


  for (size_t quality = quality_interval[0]; quality <= quality_interval[1]; quality += quality_interval[2]) {
    mse = 0;

    output.open(tmp_filename, ios::binary);
    if (!output) {
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << tmp_filename << "\" FOR WRITITNG" << endl;
      return -1;
    }

    initEncoder(encoder);
    constructQuantizationTables(encoder, "DEFAULT", quality);
    referenceScan(encoder, inputF); //FIRST IMAGE SCAN
    constructTraversalTables(encoder, "DEFAULT");
    huffmanScan(encoder, inputF); //SECOND IMAGE SCAN
    constructHuffmanTables(encoder);
    writeHeader(encoder, output);
    outputScan(encoder, inputF, output); //THIRD IMAGE SCAN

    compressed_image_size = output.tellp();

    output.close();

    input.open(tmp_filename, ios::binary);
    if (!input) {
      cerr << "ERROR: CANNON OPEN " << tmp_filename << " FOR READING\n";
      return -2;
    }

    if (readHeader(decoder, input)) {
      cerr << "ERROR: IMAGE HEADER INVALID\n";
      return -3;
    }

    initDecoder(decoder);
    decodeScan(decoder, input, outputF);

    input.close();

    double psnr = PSNR(mse / (image_pixels * 3), encoder.max_rgb_value);
    double bpp = compressed_image_size * 8.0 / image_pixels;

    data_output << quality  << " " << psnr << " " << bpp << endl;
  }

  return 0;
}

int main(int argc, char *argv[]) {
  const char *input_file_mask {};
  const char *output_file_2D  {};
  const char *output_file_3D  {};
  const char *output_file_4D  {};
  const char *quality_step    {};
  const char *quality_first   {};
  const char *quality_last    {};

  bool nothreads              {};
  bool append                 {};

  array<float, 3> quality_interval {};

  vector<uint8_t> rgb_data {};

  uint64_t width       {};
  uint64_t height      {};
  uint32_t color_depth {};
  uint64_t image_count {};

  ofstream outputs[3] {};

  vector<thread> threads {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:s:f:l:2:3:4:na")) >= 0) {
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

      case 'f':
        if (!quality_first) {
          quality_first = optarg;
          continue;
        }
      break;

      case 'l':
        if (!quality_last) {
          quality_last = optarg;
          continue;
        }
      break;

      case '2':
        if (!output_file_2D) {
          output_file_2D = optarg;
          continue;
        }
      break;

      case '3':
        if (!output_file_3D) {
          output_file_3D = optarg;
          continue;
        }
      break;

      case '4':
        if (!output_file_4D) {
          output_file_4D = optarg;
          continue;
        }
      break;

      case 'n':
        if (!nothreads) {
          nothreads = true;
          continue;
        }
      break;

      case 'a':
        if (!append) {
          append = true;
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
    cerr << "Please specify at least one argument [-2 <output-filename>] [-3 <output-filename>] [-4 <output-filename>]." << endl;
    print_usage(argv[0]);
    return 1;
  }

  quality_interval[2] = 1.0;
  if (quality_step) {
    float tmp = atof(quality_step);
    if ((tmp < 1.0) || (tmp > 100.0)) {
      print_usage(argv[0]);
      return 1;
    }
    quality_interval[2] = tmp;
  }

  quality_interval[0] = quality_interval[2];
  if (quality_first) {
    float tmp = atof(quality_first);
    if ((tmp < 1.0) || (tmp > 100.0)) {
      print_usage(argv[0]);
      return 1;
    }
    quality_interval[0] = tmp;
  }

  quality_interval[1] = 100.0;
  if (quality_last) {
    float tmp = atof(quality_last);
    if ((tmp < 1.0) || (tmp > 100.0)) {
      print_usage(argv[0]);
      return 1;
    }
    quality_interval[1] = tmp;
  }

  if (!checkPPMheaders(input_file_mask, width, height, color_depth, image_count)) {
    return 2;
  }

  size_t input_size = width * height * image_count * 3;
  input_size *= (color_depth > 255) ? 2 : 1;
  rgb_data.resize(input_size);

  if (!loadPPMs(input_file_mask, rgb_data.data())) {
    return 3;
  }

  ios_base::openmode flags { fstream::app };
  if (!append) {
    flags = fstream::trunc;
  }

  if (output_file_2D) {
    LfifEncoder<BLOCK_SIZE, 2> encoder {};

    encoder.max_rgb_value = color_depth;
    encoder.img_dims[0] = width;
    encoder.img_dims[1] = height;
    encoder.img_dims[2] = image_count;

    size_t last_slash_pos = string(output_file_2D).find_last_of('/');
    if (last_slash_pos != string::npos) {
      string command = "mkdir -p " + string(output_file_2D).substr(0, last_slash_pos);
      system(command.c_str());
    }

    outputs[0].open(output_file_2D, flags);
    if (!outputs[0]) {
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << output_file_2D << "\" FOR WRITITNG" << endl;
    }
    else {
      if (!append) {
        outputs[0] << "'2D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
      }

      if (nothreads) {
        doTest(encoder, rgb_data, quality_interval, outputs[0], "/tmp/lfifbench.lf2d");
      }
      else {
        threads.emplace_back(doTest<BLOCK_SIZE, 2>, encoder, ref(rgb_data), ref(quality_interval), ref(outputs[0]), "/tmp/lfifbench.lf2d");
      }
    }
  }

  if (output_file_3D) {
    LfifEncoder<BLOCK_SIZE, 3> encoder {};

    encoder.max_rgb_value = color_depth;
    encoder.img_dims[0] = width;
    encoder.img_dims[1] = height;
    encoder.img_dims[2] = sqrt(image_count);
    encoder.img_dims[3] = sqrt(image_count);

    size_t last_slash_pos = string(output_file_3D).find_last_of('/');
    if (last_slash_pos != string::npos) {
      string command = "mkdir -p " + string(output_file_3D).substr(0, last_slash_pos);
      system(command.c_str());
    }

    outputs[1].open(output_file_3D, flags);
    if (!outputs[1]) {
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << output_file_3D << "\" FOR WRITITNG" << endl;
    }
    else {
      if (!append) {
        outputs[1] << "'3D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
      }

      if (nothreads) {
        doTest(encoder, rgb_data, quality_interval, outputs[1], "/tmp/lfifbench.lf3d");
      }
      else {
        threads.emplace_back(doTest<BLOCK_SIZE, 3>, encoder, ref(rgb_data), ref(quality_interval), ref(outputs[1]), "/tmp/lfifbench.lf3d");
      }
    }
  }

  if (output_file_4D) {
    LfifEncoder<BLOCK_SIZE, 4> encoder {};

    encoder.max_rgb_value = color_depth;
    encoder.img_dims[0] = width;
    encoder.img_dims[1] = height;
    encoder.img_dims[2] = sqrt(image_count);
    encoder.img_dims[3] = sqrt(image_count);
    encoder.img_dims[4] = 1;

    size_t last_slash_pos = string(output_file_4D).find_last_of('/');
    if (last_slash_pos != string::npos) {
      string command = "mkdir -p " + string(output_file_4D).substr(0, last_slash_pos);
      system(command.c_str());
    }

    outputs[2].open(output_file_4D, flags);
    if (!outputs[2]) {
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << output_file_4D << "\" FOR WRITITNG" << endl;
    }
    else {
      if (!append) {
        outputs[2] << "'4D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
      }

      if (nothreads) {
        doTest(encoder, rgb_data, quality_interval, outputs[2], "/tmp/lfifbench.lf4d");
      }
      else {
        threads.emplace_back(doTest<BLOCK_SIZE, 4>, encoder, ref(rgb_data), ref(quality_interval), ref(outputs[2]), "/tmp/lfifbench.lf4d");
      }
    }
  }

  for (auto &thread: threads) {
    thread.join();
  }

  return 0;
}
