/******************************************************************************\
* SOUBOR: lf_stats.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <endian_t.h>

#include <getopt.h>

#include <cmath>

#include <iostream>
#include <iomanip>
#include <vector>

using namespace std;


void usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <file-mask>" << endl;
}

template <typename F1, typename F2>
void mae_mse(F1 &&inputF1, F2 &&inputF2, double &mae, double &mse, size_t size) {
  double sae {};
  double sse {};
  for (size_t i = 0; i < size; i++) {
    auto v1 = inputF1(i);
    auto v2 = inputF2(i);
    sae += abs(v1 - v2);
    sse += (v1 - v2) * (v1 - v2);
  }
  mse += sse / size;
  mae += sae / size;
}

template <typename F>
void lf_stats(F &&puller, size_t width, size_t height, size_t view_count, size_t max_rgb_value) {
  double mse_spatial_horizontal  { 0 };
  double mae_spatial_horizontal  { 0 };

  double mse_spatial_vertical    { 0 };
  double mae_spatial_vertical    { 0 };

  double mse_angular_horizontal  { 0 };
  double mae_angular_horizontal  { 0 };

  double mse_angular_vertical    { 0 };
  double mae_angular_vertical    { 0 };

  size_t grid_size = sqrt(view_count);
  size_t pixel_count = width * height;

  cout << "Spatial dimensions: " << grid_size << "x" << grid_size << endl;
  cout << "Angular dimensions: " << width     << "x" << height << endl;
  cout << "Color depth:        " << ceil(log2(max_rgb_value + 1)) << " bits per sample" << endl;
  cout << "                     |   MSE   |  MAE  |" << fixed << setfill(' ') << endl;
  cout << "----------------------------------------" << endl;

  for (size_t vy = 0; vy < grid_size; vy++) {
    for (size_t vx = 0; vx < grid_size - 1; vx++) {
      auto inputF1 = [&](size_t index) {
        return puller((vy * grid_size + (vx + 0)) * pixel_count * 3 + index);
      };

      auto inputF2 = [&](size_t index) {
        return puller((vy * grid_size + (vx + 1)) * pixel_count * 3 + index);
      };

      mae_mse(inputF1, inputF2, mae_spatial_horizontal, mse_spatial_horizontal, pixel_count * 3);
    }
  }

  mse_spatial_horizontal /= view_count - grid_size;
  mae_spatial_horizontal /= view_count - grid_size;

  cout << "| spatial horizontal | " << setprecision(2) << setw(7) << mse_spatial_horizontal << " | " << setw(5) << mae_spatial_horizontal << " |" << endl;

  for (size_t vy = 0; vy < grid_size - 1; vy++) {
    for (size_t vx = 0; vx < grid_size; vx++) {

      auto inputF1 = [&](size_t index) {
        return puller(((vy + 0) * grid_size + vx) * pixel_count * 3 + index);
      };

      auto inputF2 = [&](size_t index) {
        return puller(((vy + 1) * grid_size + vx) * pixel_count * 3 + index);
      };

      mae_mse(inputF1, inputF2, mae_spatial_vertical, mse_spatial_vertical, pixel_count * 3);

    }
  }

  mse_spatial_vertical /= view_count - grid_size;
  mae_spatial_vertical /= view_count - grid_size;

  cout << "| spatial vertical   | " << setprecision(2) << setw(7) << mse_spatial_vertical   << " | " << setw(5) << mae_spatial_vertical   << " |" << endl;

  for (size_t y = 0; y < height; y++) {
    for (size_t x = 0; x < width - 1; x++) {

      auto inputF1 = [&](size_t index) {
        return puller((((index / 3) * height + y) * width + (x + 0)) * 3 + (index % 3));
      };

      auto inputF2 = [&](size_t index) {
        return puller((((index / 3) * height + y) * width + (x + 1)) * 3 + (index % 3));
      };


      mae_mse(inputF1, inputF2, mae_angular_horizontal, mse_angular_horizontal, view_count * 3);
    }
  }

  mse_angular_horizontal /= pixel_count - height;
  mae_angular_horizontal /= pixel_count - height;

  cout << "| angular horizontal | " << setprecision(2) << setw(7) << mse_angular_horizontal << " | " << setw(5) << mae_angular_horizontal << " |" << endl;

  for (size_t y = 0; y < height - 1; y++) {
    for (size_t x = 0; x < width; x++) {

      auto inputF1 = [&](size_t index) {
        return puller((((index / 3) * height + (y + 0)) * width + x) * 3 + (index % 3));
      };

      auto inputF2 = [&](size_t index) {
        return puller((((index / 3) * height + (y + 1)) * width + x) * 3 + (index % 3));
      };

      mae_mse(inputF1, inputF2, mae_angular_vertical, mse_angular_vertical, view_count * 3);
    }
  }

  mse_angular_vertical /= pixel_count - width;
  mae_angular_vertical /= pixel_count - width;

  cout << "| angular vertical   | " << setprecision(2) << setw(7) << mse_angular_vertical   << " | " << setw(5) << mae_angular_vertical   << " |" << endl;
  cout << "----------------------------------------" << endl;
}

int main(int argc, char *argv[]) {
  const char *input_file_mask  {};

  vector<PPM> ppm_data         {};

  uint64_t width               {};
  uint64_t height              {};
  uint64_t view_count          {};
  uint32_t max_rgb_value       {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
          continue;
        }
        break;

      default:
        break;
    }

    usage(argv[0]);
    return 1;
  }

  if (!input_file_mask) {
    usage(argv[0]);
    return 1;
  }

  if (mapPPMs(input_file_mask, width, height, max_rgb_value, ppm_data) < 0) {
    return 2;
  }

  view_count = ppm_data.size();

  auto puller = [&](size_t index) -> uint16_t {
    size_t img       = index / (width * height * 3);
    size_t img_index = index % (width * height * 3);

    if (max_rgb_value > 255) {
      BigEndian<uint16_t> *ptr = static_cast<BigEndian<uint16_t> *>(ppm_data[img].data());
      return ptr[img_index];
    }
    else {
      BigEndian<uint8_t> *ptr = static_cast<BigEndian<uint8_t> *>(ppm_data[img].data());
      return ptr[img_index];
    }
  };

  lf_stats(puller, width, height, view_count, max_rgb_value);

  return 0;
}
