/******************************************************************************\
* SOUBOR: lfif4d_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"
#include "plenoppm.h"
#include "tiler.h"

#include <lfif_encoder.h>
#include <colorspace.h>
#include <lfif.h>

#include <cmath>

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

int main(int argc, char *argv[]) {
  const char *input_file_mask  {};
  const char *output_file_name {};

  uint8_t distortion {};
  bool    predict    {};
  bool    do_shift   {};

  if (!parse_args(argc, argv, input_file_mask, output_file_name, distortion, predict, do_shift)) {
    return 1;
  }

  std::vector<PPM> ppm_data {};

  uint64_t width         {};
  uint64_t height        {};
  uint64_t image_count   {};
  uint32_t max_rgb_value {};

  if (mapPPMs(input_file_mask, width, height, max_rgb_value, ppm_data) < 0) {
    std::cerr << "ERROR: IMAGES PROPERTIES MISMATCH\n";
    return 2;
  }

  image_count = ppm_data.size();
  if (!image_count) {
    std::cerr << "ERROR: NO IMAGES LOADED\n";
    return 2;
  }

  size_t  side        = sqrt(image_count);
  uint8_t color_depth = ceil(log2(max_rgb_value + 1));

  std::array<uint64_t, 4> image_size { width, height, side, side };
  std::array<uint64_t, 4> block_size {};
  if (optind + 4 == argc) {
    for (size_t i { 0 }; i < 4; i++) {
      std::stringstream ss {};
      ss << argv[optind++];
      size_t tmp {};
      ss >> tmp;
      if (!ss) {
        block_size = {side, side, side, side};
        break;
      }

      block_size[i] = tmp;
    }
  }
  else {
    block_size = {side, side, side, side};
  }

  auto rgb_puller = [&](const std::array<size_t, 4> &pos) -> std::array<uint16_t, 3> {
    size_t img_index = pos[1] * image_size[0] + pos[0];
    size_t img       = pos[3] * image_size[2] + pos[2];

    return ppm_data[img].get(img_index);
  };

  auto rgb_pusher = [&](const std::array<size_t, 4> &pos, const std::array<uint16_t, 3> &RGB) {
    size_t img_index = pos[1] * image_size[0] + pos[0];
    size_t img       = pos[3] * image_size[2] + pos[2];

    ppm_data[img].put(img_index, RGB);
  };

  auto yuv_puller = [&](const std::array<size_t, 4> &pos) -> std::array<float, 3> {
    std::array<uint16_t, 3> RGB = rgb_puller(pos);

    float Y  = YCbCr::RGBToY(RGB[0], RGB[1], RGB[2]) - pow(2, color_depth - 1);
    float Cb = YCbCr::RGBToCb(RGB[0], RGB[1], RGB[2]);
    float Cr = YCbCr::RGBToCr(RGB[0], RGB[1], RGB[2]);

    return {Y, Cb, Cr};
  };

  auto yuv_pusher = [&](const std::array<size_t, 4> &pos, const std::array<float, 3> &values) {
    float Y  = values[0] + pow(2, color_depth - 1);
    float Cb = values[1];
    float Cr = values[2];

    uint16_t R = std::clamp<float>(std::round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, std::pow(2, color_depth) - 1);
    uint16_t G = std::clamp<float>(std::round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, std::pow(2, color_depth) - 1);
    uint16_t B = std::clamp<float>(std::round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, std::pow(2, color_depth) - 1);

    rgb_pusher(pos, {R, G, B});
  };

  std::array<int64_t, 2> shift {};

  if (do_shift) {
    int64_t width_shift  {static_cast<int64_t>((width  * 2) / side)};
    int64_t height_shift {static_cast<int64_t>((height * 2) / side)};

    shift = find_best_shift_params(rgb_puller, image_size, {-width_shift, -height_shift}, {width_shift, height_shift}, 10, 8);

    for (size_t y = 0; y < side; y++) {
      for (size_t x = 0; x < side; x++) {
        std::array<size_t, 4> whole_image_pos {0, 0, x, y};

        auto shiftInputF = [&](const std::array<size_t, 2> &pos) {
          whole_image_pos[0] = pos[0];
          whole_image_pos[1] = pos[1];
          return rgb_puller(whole_image_pos);
        };

        auto shiftOutputF = [&](const std::array<size_t, 2> &pos, const auto &value) {
          whole_image_pos[0] = pos[0];
          whole_image_pos[1] = pos[1];
          return rgb_pusher(whole_image_pos, value);
        };

        shift_image(shiftInputF, shiftOutputF, {width, height}, get_shift_coef({x, y}, {side, side}, shift));
      }
    }
  }

  if (create_directory(output_file_name)) {
    std::cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  std::ofstream output_file {};
  output_file.open(output_file_name, std::ios::binary);
  if (!output_file) {
    std::cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  output_file << "LFIF-4D\n";
  output_file << shift[0] << " " << shift[1] << "\n";

  LFIF<4> output {};
  output.create(output_file, image_size, block_size, color_depth, distortion, predict);

  encodeStreamDCT(output_file, output, yuv_puller, yuv_pusher);

  return 0;
}
