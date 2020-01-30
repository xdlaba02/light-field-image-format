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

const char *input_file_mask {};
const char *output_file_2D  {};
const char *output_file_3D  {};
const char *output_file_4D  {};
const char *quality_step    {};
const char *quality_first   {};
const char *quality_last    {};
const char *qtabletype  = "DEFAULT";
const char *zztabletype = "DEFAULT";

std::array<size_t, 2> block_size_2D {};
std::array<size_t, 3> block_size_3D {};
std::array<size_t, 4> block_size_4D {};

bool nothreads              {};
bool append                 {};
bool huffman                {};
bool use_prediction                {};

array<float, 3> quality_interval {};

double PSNR(double mse, size_t max) {
  if (mse == .0) {
    return 0;
  }
  return 10 * log10((max * max) / mse);
}

template <size_t D>
int doTest(LfifEncoder<D> &encoder, const vector<PPM> &original, vector<PPM> &read_write, const array<float, 3> &quality_interval, ostream &data_output) {
  LfifDecoder<D> decoder       {};
  size_t image_pixels          {};
  size_t compressed_image_size {};
  double mse                   {};

  size_t width  {encoder.img_dims[0]};
  size_t height {encoder.img_dims[1]};

  uint16_t max_rgb_value = pow(2, encoder.color_depth) - 1;

  image_pixels = original.size() * width * height;

  auto inputF = [&](size_t index) -> uint16_t {
    size_t img       = index / (width * height * 3);
    size_t img_index = index % (width * height * 3);

    if (max_rgb_value > 255) {
      BigEndian<uint16_t> *ptr = static_cast<BigEndian<uint16_t> *>(read_write[img].data());
      return ptr[img_index];
    }
    else {
      BigEndian<uint8_t> *ptr = static_cast<BigEndian<uint8_t> *>(read_write[img].data());
      return ptr[img_index];
    }
  };

  auto outputF = [&](size_t index, uint16_t value) {
    size_t img       = index / (width * height * 3);
    size_t img_index = index % (width * height * 3);

    if (max_rgb_value > 255) {
      BigEndian<uint16_t> *ptr = static_cast<BigEndian<uint16_t> *>(read_write[img].data());
      ptr[img_index] = value;
    }
    else {
      BigEndian<uint8_t> *ptr = static_cast<BigEndian<uint8_t> *>(read_write[img].data());
      ptr[img_index] = value;
    }
  };

  auto puller = [&](size_t index) -> std::array<INPUTUNIT, 3> {
    uint16_t R = inputF(index * 3 + 0);
    uint16_t G = inputF(index * 3 + 1);
    uint16_t B = inputF(index * 3 + 2);

    INPUTUNIT Y  = YCbCr::RGBToY(R, G, B) - pow(2, encoder.color_depth - 1);
    INPUTUNIT Cb = YCbCr::RGBToCb(R, G, B);
    INPUTUNIT Cr = YCbCr::RGBToCr(R, G, B);

    return {Y, Cb, Cr};
  };

  auto pusher = [&](size_t index, const std::array<INPUTUNIT, 3> &values) {
    INPUTUNIT Y  = values[0] + pow(2, encoder.color_depth - 1);
    INPUTUNIT Cb = values[1];
    INPUTUNIT Cr = values[2];

    uint16_t R = clamp<INPUTUNIT>(round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, max_rgb_value);
    uint16_t G = clamp<INPUTUNIT>(round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, max_rgb_value);
    uint16_t B = clamp<INPUTUNIT>(round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, max_rgb_value);

    outputF(index * 3 + 0, R);
    outputF(index * 3 + 1, G);
    outputF(index * 3 + 2, B);
  };

  auto originalInputF = [&](size_t index) -> uint16_t {
    size_t img       = index / (width * height * 3);
    size_t img_index = index % (width * height * 3);

    if (max_rgb_value > 255) {
      const BigEndian<uint16_t> *ptr = static_cast<const BigEndian<uint16_t> *>(original[img].data());
      return ptr[img_index];
    }
    else {
      const BigEndian<uint8_t> *ptr = static_cast<const BigEndian<uint8_t> *>(original[img].data());
      return ptr[img_index];
    }
  };

  for (size_t quality = quality_interval[0]; quality <= quality_interval[1]; quality += quality_interval[2]) {
    if (use_prediction) {
      for (size_t i {}; i < image_pixels * 3; i++) {
        outputF(i, originalInputF(i));
      }
    }

    mse = 0;
    stringstream io {};

    initEncoder(encoder);
    constructQuantizationTables(encoder, qtabletype, quality);

    if (huffman) {
      if (std::string { zztabletype } == "REFERENCE") {
        referenceScan(encoder, puller);
      }
      constructTraversalTables(encoder, zztabletype);
      huffmanScan(encoder, puller);
      constructHuffmanTables(encoder);
      writeHeader(encoder, io);
      outputScanHuffman_RUNLENGTH(encoder, puller, io);
    }
    else {
      writeHeader(encoder, io);
      outputScanCABAC_DIAGONAL(encoder, puller, pusher, io);
    }

    compressed_image_size = io.tellp();

    if (use_prediction) {
      for (size_t i {}; i < image_pixels * 3; i++) {
        outputF(i, originalInputF(i));
      }
    }

    if (readHeader(decoder, io)) {
      cerr << "ERROR: IMAGE HEADER INVALID\n";
      return 2;
    }

    initDecoder(decoder);

    decodeScanCABAC(decoder, io, puller, pusher);

    for (size_t i {}; i < image_pixels * 3; i++) {
      mse += (originalInputF(i) - inputF(i)) * (originalInputF(i) - inputF(i));
    }

    double psnr = PSNR(mse / (image_pixels * 3), max_rgb_value);
    double bpp = compressed_image_size * 8.0 / image_pixels;

    data_output << quality  << " " << psnr << " " << bpp << endl;
    std::cerr << D << "D: " << quality << " " << psnr << " " << bpp << "\n";
  }

  return 0;
}

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> [-2 <output-file-name> [<x> <y>]] [-3 <output-file-name> [<x> <y> <z>]] [-4 <output-file-name> [<x> <y> <z> <ž>]] [-f <fist-quality>] [-l <last-quality>] [-s <quality-step>] [-a] [-Q <qtabletype-type>] [-Z <zztabletype-type>] [-h] [-p]" << endl;
}

void parse_args(int argc, char *argv[]) {
  char opt {};
  while ((opt = getopt(argc, argv, "i:s:f:l:2:3:4:naQ:Z:hp")) >= 0) {
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

          int tmp = atoi(argv[optind]);
          if (tmp > 0) {
            optind++;
            block_size_2D[0] = tmp;
            for (size_t i { 1 }; i < 2; i++) {
              tmp = atoi(argv[optind++]);
              if (tmp <= 0) {
                break;
              }
              block_size_2D[i] = tmp;
            }
          }

          continue;
        }
      break;

      case '3':
        if (!output_file_3D) {
          output_file_3D = optarg;

          int tmp = atoi(argv[optind]);
          if (tmp > 0) {
            optind++;
            block_size_3D[0] = tmp;
            for (size_t i { 1 }; i < 3; i++) {
              tmp = atoi(argv[optind++]);
              if (tmp <= 0) {
                break;
              }
              block_size_3D[i] = tmp;
            }
          }

          continue;
        }
      break;

      case '4':
        if (!output_file_4D) {
          output_file_4D = optarg;

          int tmp = atoi(argv[optind]);
          if (tmp > 0) {
            optind++;
            block_size_4D[0] = tmp;
            for (size_t i { 1 }; i < 4; i++) {
              tmp = atoi(argv[optind++]);
              if (tmp <= 0) {
                break;
              }
              block_size_4D[i] = tmp;
            }
          }

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

      case 'Q':
        if (string(qtabletype) == "DEFAULT") {
          qtabletype = optarg;
          continue;
        }
      break;

      case 'Z':
        if (string(zztabletype) == "DEFAULT") {
          zztabletype = optarg;
          continue;
        }
      break;

      case 'h':
        if (!huffman) {
          huffman = true;
          continue;
        }
      break;

      case 'p':
        if (!use_prediction) {
          use_prediction = true;
          continue;
        }
      break;


      default:
        print_usage(argv[0]);
        exit(1);
      break;
    }
  }

  if (!input_file_mask) {
    print_usage(argv[0]);
    exit(1);
  }

  if (!output_file_2D && !output_file_3D && !output_file_4D) {
    cerr << "Please specify at least one argument [-2 <output-filename>] [-3 <output-filename>] [-4 <output-filename>]." << endl;
    print_usage(argv[0]);
    exit(1);
  }

  quality_interval[2] = 1.0;
  if (quality_step) {
    float tmp = atof(quality_step);
    if ((tmp < 1.0) || (tmp > 100.0)) {
      print_usage(argv[0]);
      exit(1);
    }
    quality_interval[2] = tmp;
  }

  quality_interval[0] = quality_interval[2];
  if (quality_first) {
    float tmp = atof(quality_first);
    if ((tmp < 1.0) || (tmp > 100.0)) {
      print_usage(argv[0]);
      exit(1);
    }
    quality_interval[0] = tmp;
  }

  quality_interval[1] = 100.0;
  if (quality_last) {
    float tmp = atof(quality_last);
    if ((tmp < 1.0) || (tmp > 100.0)) {
      print_usage(argv[0]);
      exit(1);
    }
    quality_interval[1] = tmp;
  }
}

int main(int argc, char *argv[]) {
  uint64_t width                {};
  uint64_t height               {};
  uint32_t max_rgb_value        {};
  uint64_t image_count          {};

  vector<PPM> ppm_data_readonly {};

  ofstream outputs[3]           {};

  vector<thread> threads        {};

  LfifEncoder<2> encoder2D      {};
  LfifEncoder<3> encoder3D      {};
  LfifEncoder<4> encoder4D      {};

  parse_args(argc, argv);

  if (mapPPMs(input_file_mask, width, height, max_rgb_value, ppm_data_readonly) < 0) {
    return 2;
  }

  image_count = ppm_data_readonly.size();

  size_t block_size = sqrt(image_count);

  ios_base::openmode flags { fstream::app };
  if (!append) {
    flags = fstream::trunc;
  }

  if (output_file_2D) {
    vector<PPM> ppm_data_readwrite {};

    if (mapPPMs(input_file_mask, width, height, max_rgb_value, ppm_data_readwrite) < 0) {
      return 2;
    }

    if (!block_size_2D[0] || !block_size_2D[1]) {
      block_size_2D = { block_size, block_size };
    }

    encoder2D.block_size = block_size_2D;
    encoder2D.color_depth = ceil(log2(max_rgb_value + 1));
    encoder2D.img_dims[0] = width;
    encoder2D.img_dims[1] = height;
    encoder2D.img_dims[2] = image_count;
    encoder2D.use_huffman = huffman;
    encoder2D.use_prediction = use_prediction;

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
        doTest(encoder2D, ppm_data_readonly, ppm_data_readwrite, quality_interval, outputs[0]);
      }
      else {
        threads.emplace_back(doTest<2>, ref(encoder2D), ref(ppm_data_readonly), ref(ppm_data_readwrite), ref(quality_interval), ref(outputs[0]));
      }
    }
  }

  if (output_file_3D) {
    vector<PPM> ppm_data_readwrite {};

    if (mapPPMs(input_file_mask, width, height, max_rgb_value, ppm_data_readwrite) < 0) {
      return 2;
    }

    if (!block_size_3D[0] || !block_size_3D[1] || !block_size_3D[2]) {
      block_size_3D = { block_size, block_size, block_size };
    }

    encoder3D.block_size = block_size_3D;
    encoder3D.color_depth = ceil(log2(max_rgb_value + 1));;
    encoder3D.img_dims[0] = width;
    encoder3D.img_dims[1] = height;
    encoder3D.img_dims[2] = sqrt(image_count);
    encoder3D.img_dims[3] = sqrt(image_count);
    encoder3D.use_huffman = huffman;
    encoder3D.use_prediction = use_prediction;

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
        doTest(encoder3D, ppm_data_readonly, ppm_data_readwrite, quality_interval, outputs[1]);
      }
      else {
        threads.emplace_back(doTest<3>, ref(encoder3D), ref(ppm_data_readonly), ref(ppm_data_readwrite), ref(quality_interval), ref(outputs[1]));
      }
    }
  }

  if (output_file_4D) {
    vector<PPM> ppm_data_readwrite {};

    if (mapPPMs(input_file_mask, width, height, max_rgb_value, ppm_data_readwrite) < 0) {
      return 2;
    }

    if (!block_size_4D[0] || !block_size_4D[1] || !block_size_4D[2] || !block_size_4D[3]) {
      block_size_4D = { block_size, block_size, block_size, block_size };
    }

    encoder4D.block_size = block_size_4D;
    encoder4D.color_depth = ceil(log2(max_rgb_value + 1));;
    encoder4D.img_dims[0] = width;
    encoder4D.img_dims[1] = height;
    encoder4D.img_dims[2] = sqrt(image_count);
    encoder4D.img_dims[3] = sqrt(image_count);
    encoder4D.img_dims[4] = 1;
    encoder4D.use_huffman = huffman;
    encoder4D.use_prediction = use_prediction;

    for (size_t i {}; i < 4; i++) {
      std::cerr << encoder4D.block_size[i] << ' ';
    }
    std::cerr << '\n';

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
        doTest(encoder4D, ppm_data_readonly, ppm_data_readwrite, quality_interval, outputs[2]);
      }
      else {
        threads.emplace_back(doTest<4>, ref(encoder4D), ref(ppm_data_readonly), ref(ppm_data_readwrite), ref(quality_interval), ref(outputs[2]));
      }
    }
  }

  for (auto &thread: threads) {
    thread.join();
  }

  return 0;
}
