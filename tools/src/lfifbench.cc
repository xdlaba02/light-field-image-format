/******************************************************************************\
* SOUBOR: lfifbench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"
#include "tiler.h"

#include <colorspace.h>
#include <lfif_encoder.h>
#include <lfif_decoder.h>

#include <getopt.h>

#include <cmath>

#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

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

bool append                 {};
bool huffman                {};
bool use_prediction         {};
bool shift                  {};
bool shift_param_set        {};

std::array<int64_t, 2> shift_param {};

array<float, 3> quality_interval {};

double PSNR(double mse, size_t max) {
  if (mse == .0) {
    return 0;
  }
  return 10 * log10((max * max) / mse);
}

template <size_t D>
int doTest(LfifEncoder<D> &encoder, const vector<PPM> &original, vector<PPM> &read_write, const array<float, 3> &quality_interval, ostream &data_output) {
  size_t image_pixels          {};

  size_t width  {encoder.img_dims[0]};
  size_t height {encoder.img_dims[1]};

  uint16_t max_rgb_value = pow(2, encoder.color_depth) - 1;

  image_pixels = original.size() * width * height;

  auto rgb_puller = [&](const std::array<size_t, D + 1> &pos) -> std::array<uint16_t, 3> {
    size_t img_index {};
    size_t img       {};

    img_index = pos[1] * encoder.img_dims[0] + pos[0];

    for (size_t i { 0 }; i <= D - 2; i++) {
      size_t idx = D - i;
      img *= encoder.img_dims[idx];
      img += pos[idx];
    }

    return read_write[img].get(img_index);
  };

  auto rgb_pusher = [&](const std::array<size_t, D + 1> &pos, const std::array<uint16_t, 3> &RGB) {
    size_t img_index {};
    size_t img       {};

    img_index = pos[1] * encoder.img_dims[0] + pos[0];

    for (size_t i { 0 }; i <= D - 2; i++) {
      size_t idx = D - i;
      img *= encoder.img_dims[idx];
      img += pos[idx];
    }

    read_write[img].put(img_index, RGB);
  };

  auto yuv_puller = [&](const std::array<size_t, D + 1> &pos) -> std::array<INPUTUNIT, 3> {
    std::array<uint16_t, 3> RGB = rgb_puller(pos);

    INPUTUNIT Y  = YCbCr::RGBToY(RGB[0], RGB[1], RGB[2]) - pow(2, encoder.color_depth - 1);
    INPUTUNIT Cb = YCbCr::RGBToCb(RGB[0], RGB[1], RGB[2]);
    INPUTUNIT Cr = YCbCr::RGBToCr(RGB[0], RGB[1], RGB[2]);

    return {Y, Cb, Cr};
  };

  auto yuv_pusher = [&](const std::array<size_t, D + 1> &pos, const std::array<INPUTUNIT, 3> &values) {
    INPUTUNIT Y  = values[0] + pow(2, encoder.color_depth - 1);
    INPUTUNIT Cb = values[1];
    INPUTUNIT Cr = values[2];

    uint16_t R = clamp<INPUTUNIT>(round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, pow(2, encoder.color_depth) - 1);
    uint16_t G = clamp<INPUTUNIT>(round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, pow(2, encoder.color_depth) - 1);
    uint16_t B = clamp<INPUTUNIT>(round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, pow(2, encoder.color_depth) - 1);

    rgb_pusher(pos, {R, G, B});
  };

  auto original_rgb_puller = [&](const std::array<size_t, D + 1> &pos) -> std::array<uint16_t, 3> {
    size_t img_index {};
    size_t img       {};

    img_index = pos[1] * encoder.img_dims[0] + pos[0];

    for (size_t i { 0 }; i <= D - 2; i++) {
      size_t idx = D - i;
      img *= encoder.img_dims[idx];
      img += pos[idx];
    }

    return original[img].get(img_index);
  };

  for (size_t quality = quality_interval[0]; quality <= quality_interval[1]; quality += quality_interval[2]) {
    stringstream io  {};
    double       mse {};

    iterate_dimensions<D + 1>(encoder.img_dims, [&](const auto &pos) {
      rgb_pusher(pos, original_rgb_puller(pos));
    });

    initEncoder(encoder);
    constructQuantizationTables(encoder, qtabletype, quality);

    if (huffman) {
      if (std::string { zztabletype } == "REFERENCE") {
        referenceScan(encoder, yuv_puller);
      }
      constructTraversalTables(encoder, zztabletype);
      huffmanScan(encoder, yuv_puller);
      constructHuffmanTables(encoder);
      writeHeader(encoder, io);
      outputScanHuffman_RUNLENGTH(encoder, yuv_puller, io);
    }
    else {
      writeHeader(encoder, io);
      outputScanCABAC_DIAGONAL(encoder, yuv_puller, yuv_pusher, io);
    }

    size_t compressed_image_size = io.tellp();

    iterate_dimensions<D + 1>(encoder.img_dims, [&](const auto &pos) {
      auto original = original_rgb_puller(pos);
      auto decoded  = rgb_puller(pos);

      mse += (original[0] - decoded[0]) * (original[0] - decoded[0]);
      mse += (original[1] - decoded[1]) * (original[1] - decoded[1]);
      mse += (original[2] - decoded[2]) * (original[2] - decoded[2]);
    });

    double psnr = PSNR(mse / (image_pixels * 3), max_rgb_value);
    double bpp = compressed_image_size * 8.0 / image_pixels;

    data_output << quality  << " " << psnr << " " << bpp << endl;
    std::cerr << D << "D: " << quality << " " << psnr << " " << bpp << "\n";
  }

  return 0;
}

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> [-2 <output-file-name> [<x> <y>]] [-3 <output-file-name> [<x> <y> <z>]] [-4 <output-file-name> [<x> <y> <z> <ž>]] [-f <fist-quality>] [-l <last-quality>] [-s <quality-step>] [-a] [-Q <qtabletype-type>] [-Z <zztabletype-type>] [-h] [-p] [-t [<x> <y>]]" << endl;
}

void parse_args(int argc, char *argv[]) {
  char opt {};
  while ((opt = getopt(argc, argv, "i:s:f:l:2:3:4:aQ:Z:hpt")) >= 0) {
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

          if (optind + 1 < argc) {
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
          }

          continue;
        }
      break;

      case '3':
        if (!output_file_3D) {
          output_file_3D = optarg;

          if (optind + 2 < argc) {
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
          }

          continue;
        }
      break;

      case '4':
        if (!output_file_4D) {
          output_file_4D = optarg;

          if (optind + 3 < argc) {
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
          }

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

      case 't':
        if (!shift) {
          shift = true;

          if (optind + 1 < argc) {
            auto shift_x = std::stringstream(argv[optind]);
            shift_x >> shift_param[0];
            if (!shift_x.fail()) {
              auto shift_y = std::stringstream(argv[optind + 1]);
              shift_y >> shift_param[1];
              if (!shift_y.fail()) {
                optind += 2;
                shift_param_set = true;
              }
            }
          }
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

  parse_args(argc, argv);

  if (mapPPMs(input_file_mask, width, height, max_rgb_value, ppm_data_readonly) < 0) {
    return 2;
  }

  image_count = ppm_data_readonly.size();

  size_t side = sqrt(image_count);

  auto shift_images = [&](std::vector<PPM> &ppms) {
    if (shift) {
      auto inputF = [&](const std::array<size_t, 4> &pos) -> std::array<uint16_t, 3> {
        size_t img_index = pos[1] * width + pos[0];
        size_t img       = pos[3] * side  + pos[2];

        return ppms[img].get(img_index);
      };

      if (!shift_param_set) {
        int64_t width_shift  {static_cast<int64_t>((width  * 2) / side)};
        int64_t height_shift {static_cast<int64_t>((height * 2) / side)};

        shift_param = find_best_shift_params(inputF, {width, height, side, side}, {-width_shift, -height_shift}, {width_shift, height_shift}, 10, 8);
        std::cerr << shift_param[0] << " " << shift_param[1] << "\n";
        shift_param_set = true;
      }

      for (size_t y {}; y < side; y++) {
        for (size_t x {}; x < side; x++) {
          auto shiftInputF = [&](const std::array<size_t, 2> &pos) {
            std::array<size_t, 4> whole_image_pos {};

            whole_image_pos[0] = pos[0];
            whole_image_pos[1] = pos[1];
            whole_image_pos[2] = x;
            whole_image_pos[3] = y;

            return inputF(whole_image_pos);
          };

          auto shiftOutputF = [&](const std::array<size_t, 2> &pos, const auto &value) {
            size_t img_index = pos[1] * width + pos[0];
            size_t img       = y * side + x;

            return ppms[img].put(img_index, value);
          };

          shift_image(shiftInputF, shiftOutputF, {width, height}, get_shift_coef({x, y}, {side, side}, shift_param));
        }
      }
    }
  };

  ios_base::openmode flags { fstream::app };
  if (!append) {
    flags = fstream::trunc;
  }

  shift_images(ppm_data_readonly);

  if (output_file_2D) {
    LfifEncoder<2> encoder2D       {};
    vector<PPM> ppm_data_readwrite {};

    if (mapPPMs(input_file_mask, width, height, max_rgb_value, ppm_data_readwrite) < 0) {
      return 2;
    }

    shift_images(ppm_data_readwrite);

    if (!block_size_2D[0] || !block_size_2D[1]) {
      block_size_2D = { side, side };
    }

    encoder2D.block_size = block_size_2D;
    encoder2D.color_depth = ceil(log2(max_rgb_value + 1));
    encoder2D.img_dims[0] = width;
    encoder2D.img_dims[1] = height;
    encoder2D.img_dims[2] = image_count;
    encoder2D.use_huffman = huffman;
    encoder2D.use_prediction = use_prediction;


    if (create_directory(output_file_2D)) {
      cerr << "ERROR: CANNON OPEN " << output_file_2D << " FOR WRITING\n";
      return 1;
    }

    outputs[0].open(output_file_2D, flags);
    if (!outputs[0]) {
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << output_file_2D << "\" FOR WRITITNG" << endl;
    }
    else {
      if (!append) {
        outputs[0] << "'2D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
      }

      doTest(encoder2D, ppm_data_readonly, ppm_data_readwrite, quality_interval, outputs[0]);
    }
  }

  if (output_file_3D) {
    LfifEncoder<3> encoder3D       {};
    vector<PPM> ppm_data_readwrite {};

    if (mapPPMs(input_file_mask, width, height, max_rgb_value, ppm_data_readwrite) < 0) {
      return 2;
    }

    shift_images(ppm_data_readwrite);

    if (!block_size_3D[0] || !block_size_3D[1] || !block_size_3D[2]) {
      block_size_3D = { side, side, side };
    }

    encoder3D.block_size = block_size_3D;
    encoder3D.color_depth = ceil(log2(max_rgb_value + 1));;
    encoder3D.img_dims[0] = width;
    encoder3D.img_dims[1] = height;
    encoder3D.img_dims[2] = sqrt(image_count);
    encoder3D.img_dims[3] = sqrt(image_count);
    encoder3D.use_huffman = huffman;
    encoder3D.use_prediction = use_prediction;

    if (create_directory(output_file_3D)) {
      cerr << "ERROR: CANNON OPEN " << output_file_3D << " FOR WRITING\n";
      return 1;
    }

    outputs[1].open(output_file_3D, flags);
    if (!outputs[1]) {
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << output_file_3D << "\" FOR WRITITNG" << endl;
    }
    else {
      if (!append) {
        outputs[1] << "'3D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
      }

      doTest(encoder3D, ppm_data_readonly, ppm_data_readwrite, quality_interval, outputs[1]);
    }
  }

  if (output_file_4D) {
    LfifEncoder<4> encoder4D       {};
    vector<PPM> ppm_data_readwrite {};

    if (mapPPMs(input_file_mask, width, height, max_rgb_value, ppm_data_readwrite) < 0) {
      return 2;
    }

    shift_images(ppm_data_readwrite);

    if (!block_size_4D[0] || !block_size_4D[1] || !block_size_4D[2] || !block_size_4D[3]) {
      block_size_4D = { side, side, side, side };
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

    if (create_directory(output_file_4D)) {
      cerr << "ERROR: CANNON OPEN " << output_file_4D << " FOR WRITING\n";
      return 1;
    }

    outputs[2].open(output_file_4D, flags);
    if (!outputs[2]) {
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << output_file_4D << "\" FOR WRITITNG" << endl;
    }
    else {
      if (!append) {
        outputs[2] << "'4D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
      }

      doTest(encoder4D, ppm_data_readonly, ppm_data_readwrite, quality_interval, outputs[2]);
    }
  }

  return 0;
}
