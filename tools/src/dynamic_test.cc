/******************************************************************************\
* SOUBOR: dynamix_test.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <dct.h>

#include <iomanip>
#include <iostream>

using namespace std;

const size_t D = 2;

int main(void) {
  const size_t dims[D] = {3, 8};

  DynamicBlock<DCTDATAUNIT, D> input_block(dims);
  DynamicBlock<DCTDATAUNIT, D> output_block(dims);

  for (size_t i { 0 }; i < get_stride<D>(dims); i++) {
    input_block[i] = i;
  }

  for (size_t z = 0; z < 1; z++) {
    for (size_t y = 0; y < dims[1]; y++) {
      for (size_t x = 0; x < dims[0]; x++) {
        std::cout << std::setw(4) << input_block[(z * dims[1] + y) * dims[0] + x];
      }
      std::cout << '\n';
    }
    std::cout << '\n';
  }
  std::cout << '\n';

  auto inputF = [&](size_t index) -> auto & {
    return input_block[index];
  };

  auto outputF = [&](size_t index) -> auto & {
    return output_block[index];
  };

  fdct<D>(dims, inputF, outputF);

  for (size_t z = 0; z < 1; z++) {
    for (size_t y = 0; y < dims[1]; y++) {
      for (size_t x = 0; x < dims[0]; x++) {
        std::cout << std::setw(6) << std::fixed << std::setprecision(1) << output_block[(z * dims[1] + y) * dims[0] + x];
      }
      std::cout << '\n';
    }
    std::cout << '\n';
  }
  std::cout << '\n';

  std::fill(&input_block[0], &input_block[get_stride<D>(dims)], 0);

  idct<D>(dims, outputF, inputF);

  for (size_t z = 0; z < 1; z++) {
    for (size_t y = 0; y < dims[1]; y++) {
      for (size_t x = 0; x < dims[0]; x++) {
        std::cout << std::setw(6) << std::fixed << std::setprecision(1) << input_block[(z * dims[1] + y) * dims[0] + x];
      }
      std::cout << '\n';
    }
    std::cout << '\n';
  }
  std::cout << '\n';
}
