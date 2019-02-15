/******************************************************************************\
* SOUBOR: lfifbench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "lfifbench.h"
#include "plenoppm.h"

#include <getopt.h>

#include <cmath>

#include <thread>

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> [-2 <output-file-name>] [-3 <output-file-name>] [-4 <output-file-name>] [-s <quality-step>]" << endl;
}

double PSNR(const RGBData &original, const RGBData &compared) {
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

int method2D(const RGBData &original, uint64_t width, uint64_t height, uint64_t image_count, uint8_t q_step, const char *output_filename) {
  ofstream output(output_filename);
  if (output.fail()) {
    return 1;
  }

  output << "'2D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;

  size_t image_pixels = width * height * image_count;

  for (size_t quality = q_step; quality <= 100; quality += q_step) {
    cerr << "LFIF2D: Q" << quality << " STARTED" << endl;
    size_t   compressed_image_size    {};
    RGBData  decompressed_rgb_data    {};
    uint64_t decompressed_image_count {};
    uint64_t img_dims[2]              {width, height};

    compressed_image_size = encode<2>(original, img_dims, image_count, quality, "/tmp/lfifbench.lfi2d");
    if (!compressed_image_size) {
      return 3;
    }

    if (decode<2>("/tmp/lfifbench.lfi2d", decompressed_rgb_data, img_dims, decompressed_image_count)) {
      return 3;
    }

    cerr << "LFIF2D: " << quality  << " " << PSNR(original, decompressed_rgb_data) << " " << compressed_image_size * 8.0 / image_pixels << endl;
    output << quality  << " " << PSNR(original, decompressed_rgb_data) << " " << compressed_image_size * 8.0 / image_pixels << endl;
  }

  return 0;
}

int method3D(const RGBData &original, uint64_t width, uint64_t height, uint64_t image_count, uint8_t q_step, const char *output_filename) {
  ofstream output(output_filename);
  if (output.fail()) {
    return 1;
  }

  output << "'3D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;

  size_t image_pixels = width * height * image_count;

  for (size_t quality = q_step; quality <= 100; quality += q_step) {
    cerr << "LFIF3D: Q" << quality << " STARTED" << endl;
    size_t   compressed_image_size    {};
    RGBData  decompressed_rgb_data    {};
    uint64_t decompressed_image_count {};
    uint64_t img_dims[3]              {width, height, image_count};

    compressed_image_size = encode<3>(original, img_dims, 1, quality, "/tmp/lfifbench.lfi3d");
    if (!compressed_image_size) {
      return 3;
    }

    if (decode<3>("/tmp/lfifbench.lfi3d", decompressed_rgb_data, img_dims, decompressed_image_count)) {
      return 3;
    }

    cerr << "LFIF3D: " << quality  << " " << PSNR(original, decompressed_rgb_data) << " " << compressed_image_size * 8.0 / image_pixels << endl;
    output << quality  << " " << PSNR(original, decompressed_rgb_data) << " " << compressed_image_size * 8.0 / image_pixels << endl;
  }

  return 0;
}

int method4D(const RGBData &original, uint64_t width, uint64_t height, uint64_t image_count, uint8_t q_step, const char *output_filename) {
  ofstream output(output_filename);
  if (output.fail()) {
    return 1;
  }

  output << "'4D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;

  size_t image_pixels = width * height * image_count;

  for (size_t quality = q_step; quality <= 100; quality += q_step) {
    cerr << "LFIF4D: Q" << quality << " STARTED" << endl;
    size_t   compressed_image_size    {};
    RGBData  decompressed_rgb_data    {};
    uint64_t decompressed_image_count {};
    uint64_t img_dims[4]              {width, height, static_cast<uint64_t>(sqrt(image_count)), static_cast<uint64_t>(sqrt(image_count))};

    compressed_image_size = encode<4>(original, img_dims, 1, quality, "/tmp/lfifbench.lfi4d");
    if (!compressed_image_size) {
      return 3;
    }

    if (decode<4>("/tmp/lfifbench.lfi4d", decompressed_rgb_data, img_dims, decompressed_image_count)) {
      return 3;
    }

    cerr << "LFIF4D: " << quality  << " " << PSNR(original, decompressed_rgb_data) << " " << compressed_image_size * 8.0 / image_pixels << endl;
    output << quality  << " " << PSNR(original, decompressed_rgb_data) << " " << compressed_image_size * 8.0 / image_pixels << endl;
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

  if (nothreads) {
    if (output_file_2D) {
      method2D(rgb_data, width, height, image_count, q_step, output_file_2D);
    }
    if (output_file_3D) {
      method3D(rgb_data, width, height, image_count, q_step, output_file_3D);
    }
    if (output_file_4D) {
      method4D(rgb_data, width, height, image_count, q_step, output_file_4D);
    }
  }
  else {
    vector<thread> threads {};

    if (output_file_2D) {
      threads.push_back(thread(method2D, rgb_data, width, height, image_count, q_step, output_file_2D));
    }

    if (output_file_3D) {
      threads.push_back(thread(method3D, rgb_data, width, height, image_count, q_step, output_file_3D));
    }

    if (output_file_4D) {
      threads.push_back(thread(method4D, rgb_data, width, height, image_count, q_step, output_file_4D));
    }

    for (auto &thread: threads) {
      thread.join();
    }
  }

  return 0;
}
