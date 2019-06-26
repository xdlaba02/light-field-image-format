/******************************************************************************\
* SOUBOR: lf_psnr.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <zigzag.h>

#include <getopt.h>

#include <cmath>

#include <iostream>
#include <iomanip>
#include <vector>

using namespace std;

vector<size_t> generateZigzagTable(size_t width, size_t height) {
  vector<size_t> table(width * height);

  size_t x = 0;
  size_t y = 0;
  size_t output_index = 0;

  while (true) {
    table[y * width + x] = output_index++;

    if (x < width - 1) {
      x++;
    }
    else if (y < height - 1) {
      y++;
    }
    else {
      break;
    }

    while ((x > 0) && (y < height - 1)) {
      table[y * width + x] = output_index++;
      x--;
      y++;
    }

    table[y * width + x] = output_index++;

    if (y < height - 1) {
      y++;
    }
    else if (x < width - 1) {
      x++;
    }
    else {
      break;
    }

    while ((x < width - 1) && (y > 0)) {
      table[y * width + x] = output_index++;
      x++;
      y--;
    }
  }

  return table;
}

void usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <file-mask>" << endl;
}

template <typename F1, typename F2>
double mse(F1 &&inputF1, F2 &&inputF2, size_t size) {
  double mse { 0 };
  for (size_t i = 0; i < size; i++) {
    auto v1 = inputF1(i);
    auto v2 = inputF2(i);
    mse += (v1 - v2) * (v1 - v2);
  }
  return mse / size;
}

template <typename F1, typename F2>
double mae(F1 &&inputF1, F2 &&inputF2, size_t size) {
  double mae { 0 };
  for (size_t i = 0; i < size; i++) {
    auto v1 = inputF1(i);
    auto v2 = inputF2(i);
    mae += abs(v1 - v2);
  }
  return mae / size;
}

double psnr(double mse, size_t max) {
  if (mse) {
    return 10 * log10((max * max) / mse);
  }

  return 0;
}

template <typename T>
void views_psnr(T *rgb_data, size_t width, size_t height, size_t view_count, size_t max_rgb_value) {
  double mse_spatial_horizontal  { 0 };
  double mae_spatial_horizontal  { 0 };

  double mse_spatial_vertical    { 0 };
  double mae_spatial_vertical    { 0 };

  double mse_angular_horizontal  { 0 };
  double mae_angular_horizontal  { 0 };

  double mse_angular_vertical    { 0 };
  double mae_angular_vertical    { 0 };

  double mse_spatial_horizontal_wrapped  { 0 };
  double mae_spatial_horizontal_wrapped  { 0 };

  double mse_angular_horizontal_wrapped  { 0 };
  double mae_angular_horizontal_wrapped  { 0 };

  double mse_spatial_zigzag  { 0 };
  double mae_spatial_zigzag  { 0 };

  vector<size_t> zigzag_table_spatial          {};
  vector<size_t> zigzag_table_spatial_inverted {};

  double mse_angular_zigzag  { 0 };
  double mae_angular_zigzag  { 0 };

  vector<size_t> zigzag_table_angular          {};
  vector<size_t> zigzag_table_angular_inverted {};

  size_t grid_size = sqrt(view_count);
  size_t pixel_count = width * height;

  cout << "Spatial dimensions: " << grid_size << "x" << grid_size << endl;
  cout << "Angular dimensions: " << width     << "x" << height << endl;
  cout << "Color depth:        " << ceil(log2(max_rgb_value + 1)) << " bits per sample" << endl;

  for (size_t vy = 0; vy < grid_size; vy++) {
    for (size_t vx = 0; vx < grid_size - 1; vx++) {

      auto inputF1 = [&](size_t index) {
        return rgb_data[(vy * grid_size + vx)       * pixel_count * 3 + index];
      };

      auto inputF2 = [&](size_t index) {
        return rgb_data[(vy * grid_size + (vx + 1)) * pixel_count * 3 + index];
      };

      mse_spatial_horizontal += mse(inputF1, inputF2, pixel_count * 3);
      mae_spatial_horizontal += mae(inputF1, inputF2, pixel_count * 3);
    }
  }

  mse_spatial_horizontal /= view_count - grid_size;
  mae_spatial_horizontal /= view_count - grid_size;

  for (size_t vy = 0; vy < grid_size - 1; vy++) {
    for (size_t vx = 0; vx < grid_size; vx++) {

      auto inputF1 = [&](size_t index) {
        return rgb_data[(vy       * grid_size + vx) * pixel_count * 3 + index];
      };

      auto inputF2 = [&](size_t index) {
        return rgb_data[((vy + 1) * grid_size + vx) * pixel_count * 3 + index];
      };

      mse_spatial_vertical += mse(inputF1, inputF2, pixel_count * 3);
      mae_spatial_vertical += mae(inputF1, inputF2, pixel_count * 3);
    }
  }

  mse_spatial_vertical /= view_count - grid_size;
  mae_spatial_vertical /= view_count - grid_size;

  for (size_t y = 0; y < height; y++) {
    for (size_t x = 0; x < width - 1; x++) {

      auto inputF1 = [&](size_t index) {
        return rgb_data[(((index / 3) * height + y) * width + x) * 3 + (index % 3)];
      };

      auto inputF2 = [&](size_t index) {
        return rgb_data[(((index / 3) * height + y) * width + (x + 1)) * 3 + (index % 3)];
      };

      mse_angular_horizontal += mse(inputF1, inputF2, view_count * 3);
      mae_angular_horizontal += mae(inputF1, inputF2, view_count * 3);
    }
  }

  mse_angular_horizontal /= pixel_count - height;
  mae_angular_horizontal /= pixel_count - height;

  for (size_t y = 0; y < height - 1; y++) {
    for (size_t x = 0; x < width; x++) {

      auto inputF1 = [&](size_t index) {
        return rgb_data[(((index / 3) * height + y) * width + x) * 3 + (index % 3)];
      };

      auto inputF2 = [&](size_t index) {
        return rgb_data[(((index / 3) * height + (y + 1)) * width + x) * 3 + (index % 3)];
      };

      mse_angular_vertical += mse(inputF1, inputF2, view_count * 3);
      mae_angular_vertical += mae(inputF1, inputF2, view_count * 3);
    }
  }

  mse_angular_vertical /= pixel_count - width;
  mae_angular_vertical /= pixel_count - width;

  cout << "                           | PSNR | MAE |" << fixed << setfill(' ') << endl;
  cout << "-----------------------------------------" << endl;
  cout << "|spatial horizontal        |" << setprecision(2) << setw(6) << psnr(mse_spatial_horizontal, max_rgb_value) << "|" << setw(4) << mae_spatial_horizontal * 100 / max_rgb_value << "%|" << endl;
  cout << "|spatial vertical          |" << setprecision(2) << setw(6) << psnr(mse_spatial_vertical, max_rgb_value)   << "|" << setw(4) << mae_spatial_vertical   * 100 / max_rgb_value << "%|" << endl;
  cout << "|angular horizontal        |" << setprecision(2) << setw(6) << psnr(mse_angular_horizontal, max_rgb_value) << "|" << setw(4) << mae_angular_horizontal * 100 / max_rgb_value << "%|" << endl;
  cout << "|angular vertical          |" << setprecision(2) << setw(6) << psnr(mse_angular_vertical, max_rgb_value)   << "|" << setw(4) << mae_angular_vertical   * 100 / max_rgb_value << "%|" << endl;
  cout << "-----------------------------------------" << endl;

  for (size_t vy = 0; vy < grid_size - 1; vy++) {
    auto inputF1 = [&](size_t index) {
      return rgb_data[(vy * grid_size + (grid_size - 1))       * pixel_count * 3 + index];
    };

    auto inputF2 = [&](size_t index) {
      return rgb_data[(vy + 1) * grid_size * pixel_count * 3 + index];
    };

    mse_spatial_horizontal_wrapped += mse(inputF1, inputF2, pixel_count * 3);
    mae_spatial_horizontal_wrapped += mae(inputF1, inputF2, pixel_count * 3);
  }

  mse_spatial_horizontal_wrapped = (mse_spatial_horizontal_wrapped + mse_spatial_horizontal * (view_count - grid_size)) / (view_count - 1);
  mae_spatial_horizontal_wrapped = (mae_spatial_horizontal_wrapped + mae_spatial_horizontal * (view_count - grid_size)) / (view_count - 1);

  zigzag_table_spatial = generateZigzagTable(grid_size, grid_size);
  zigzag_table_spatial_inverted.resize(zigzag_table_spatial.size());

  for (size_t i = 0; i < view_count; i++) {
    zigzag_table_spatial_inverted[zigzag_table_spatial[i]] = i;
  }

  for (size_t i = 0; i < view_count - 1; i++) {
    auto inputF1 = [&](size_t index) {
      return rgb_data[zigzag_table_spatial_inverted[i]     * pixel_count * 3 + index];
    };

    auto inputF2 = [&](size_t index) {
      return rgb_data[zigzag_table_spatial_inverted[i + 1] * pixel_count * 3 + index];
    };

    mse_spatial_zigzag += mse(inputF1, inputF2, pixel_count * 3);
    mae_spatial_zigzag += mae(inputF1, inputF2, pixel_count * 3);
  }

  mse_spatial_zigzag /= view_count - 1;
  mae_spatial_zigzag /= view_count - 1;

  for (size_t y = 0; y < height; y++) {
    auto inputF1 = [&](size_t index) {
      return rgb_data[(((index / 3) * height + y) * width + (width - 1)) * 3 + (index % 3)];
    };

    auto inputF2 = [&](size_t index) {
      return rgb_data[((index / 3) * height + (y + 1)) * width * 3 + (index % 3)];
    };

    mse_angular_horizontal_wrapped += mse(inputF1, inputF2, view_count * 3);
    mae_angular_horizontal_wrapped += mae(inputF1, inputF2, view_count * 3);
  }

  mse_angular_horizontal_wrapped = (mse_angular_horizontal_wrapped + mse_angular_horizontal * (pixel_count - height)) / (pixel_count - 1);
  mae_angular_horizontal_wrapped = (mae_angular_horizontal_wrapped + mae_angular_horizontal * (pixel_count - height)) / (pixel_count - 1);

  zigzag_table_angular = generateZigzagTable(width, height);
  zigzag_table_angular_inverted.resize(zigzag_table_angular.size());

  for (size_t i = 0; i < pixel_count; i++) {
    zigzag_table_angular_inverted[zigzag_table_angular[i]] = i;
  }

  for (size_t i = 0; i < pixel_count; i++) {
    auto inputF1 = [&](size_t index) {
      return rgb_data[((index / 3) * pixel_count + zigzag_table_angular_inverted[i])     * 3 + (index % 3)];
    };

    auto inputF2 = [&](size_t index) {
      return rgb_data[((index / 3) * pixel_count + zigzag_table_angular_inverted[i + 1]) * 3 + (index % 3)];
    };

    mse_angular_zigzag += mse(inputF1, inputF2, view_count * 3);
    mae_angular_zigzag += mae(inputF1, inputF2, view_count * 3);
  }

  mse_angular_zigzag /= pixel_count - 1;
  mae_angular_zigzag /= pixel_count - 1;

  cout << "|spatial horizontal wrapped|" << setprecision(2) << setw(6) << psnr(mse_spatial_horizontal_wrapped, max_rgb_value) << "|" << setw(4) << mae_spatial_horizontal_wrapped * 100 / max_rgb_value << "%|" << endl;
  cout << "|spatial zigzag            |" << setprecision(2) << setw(6) << psnr(mse_spatial_zigzag, max_rgb_value)             << "|" << setw(4) << mae_spatial_zigzag             * 100 / max_rgb_value << "%|" << endl;
  cout << "|angular horizontal wrapped|" << setprecision(2) << setw(6) << psnr(mse_angular_horizontal_wrapped, max_rgb_value) << "|" << setw(4) << mae_angular_horizontal_wrapped * 100 / max_rgb_value << "%|" << endl;
  cout << "|angular zigzag            |" << setprecision(2) << setw(6) << psnr(mse_angular_zigzag, max_rgb_value)             << "|" << setw(4) << mae_angular_zigzag             * 100 / max_rgb_value << "%|" << endl;
  cout << "-----------------------------------------" << endl;
}

int main(int argc, char *argv[]) {
  const char *input_file_mask  {};

  vector<uint8_t> rgb_data     {};

  uint64_t width               {};
  uint64_t height              {};
  uint64_t view_count         {};
  uint32_t max_rgb_value       {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:o:q:")) >= 0) {
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

  if (!checkPPMheaders(input_file_mask, width, height, max_rgb_value, view_count)) {
    return 2;
  }

  size_t bytes_per_sample = (max_rgb_value > 255) ? 2 : 1;

  size_t pixel_count = width * height * 3;
  size_t image_size = pixel_count * view_count;

  rgb_data.resize(image_size * bytes_per_sample);

  if (!loadPPMs(input_file_mask, rgb_data.data())) {
    return 2;
  }

  if (max_rgb_value < 256) {
    views_psnr(reinterpret_cast<uint8_t *>(rgb_data.data()), width, height, view_count, max_rgb_value);
  }
  else {
    views_psnr(reinterpret_cast<uint16_t *>(rgb_data.data()), width, height, view_count, max_rgb_value);
  }

  return 0;
}
