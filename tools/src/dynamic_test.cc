/******************************************************************************\
* SOUBOR: dynamix_test.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <block.h>

#include <iomanip>
#include <iostream>

using namespace std;

const size_t D = 2;

int main(void) {
  const std::array<size_t, D> dims_input = {5, 10};
  const std::array<size_t, D> dims = {10, 5};

  DynamicBlock<int, D> input_block(dims_input.data());
  DynamicBlock<int, D> output_block(dims.data());

  for (size_t i { 0 }; i < get_stride<D>(dims_input.data()); i++) {
    input_block[i] = i;
  }

  for (size_t z = 0; z < 1; z++) {
    for (size_t y = 0; y < dims_input[1]; y++) {
      for (size_t x = 0; x < dims_input[0]; x++) {
        std::cout << std::setw(4) << input_block[(z * dims_input[1] + y) * dims_input[0] + x];
      }
      std::cout << '\n';
    }
    std::cout << '\n';
  }
  std::cout << '\n';

  auto inputF = [&](const std::array<size_t, D> &img_pos) {
    return input_block[img_pos];
  };

  auto outputF = [&](const std::array<size_t, D> &block_pos, const auto &value) {
    output_block[block_pos] = value;
  };

  std::array<size_t, D> pos {};
  getBlock<D>(dims.data(), inputF, pos, dims_input, outputF);

  for (size_t z = 0; z < 1; z++) {
    for (size_t y = 0; y < dims[1]; y++) {
      for (size_t x = 0; x < dims[0]; x++) {
        std::cout << std::setw(6) << std::fixed << std::setprecision(1) << output_block[(z * dims[1] + y) * dims[0] + x];
        output_block[(z * dims[1] + y) * dims[0] + x] = 0;
      }
      std::cout << '\n';
    }
    std::cout << '\n';
  }
  std::cout << '\n';

  auto inputF2 = [&](const std::array<size_t, D> &img_pos) {
    return output_block[img_pos];
  };

  auto outputF2 = [&](const std::array<size_t, D> &block_pos, const auto &value) {
    input_block[block_pos] = value;
  };

  putBlock<D>(dims.data(), inputF2, {}, dims_input, outputF2);

  for (size_t z = 0; z < 1; z++) {
    for (size_t y = 0; y < dims_input[1]; y++) {
      for (size_t x = 0; x < dims_input[0]; x++) {
        std::cout << std::setw(6) << std::fixed << std::setprecision(1) << input_block[(z * dims_input[1] + y) * dims_input[0] + x];
      }
      std::cout << '\n';
    }
    std::cout << '\n';
  }
  std::cout << '\n';
}
