/******************************************************************************\
* SOUBOR: predict_test.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <predict.h>

#include <iomanip>
#include <iostream>

using namespace std;

const size_t BS = 8;
const size_t D = 3;

int main(void) {
  Block<INPUTUNIT, 2 * BS, D> test_block {};
  Block<INPUTUNIT, BS, D> predicted_block {};
  std::array<size_t, D> block_dims {};

  for (size_t i { 0 }; i < constpow(2 * BS, D); i++) {
    test_block[i] = i;
  }

  for (size_t z = 0; z < BS * 2; z++) {
    for (size_t y = 0; y < BS * 2; y++) {
      for (size_t x = 0; x < BS * 2; x++) {
        std::cout << std::setw(5) << test_block[(z * BS * 2 + y) * BS * 2 + x];
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;

  block_dims.fill(2);

  predict_perpendicular<BS, D>(predicted_block, block_dims.data(), &test_block[4 * constpow(BS, D) + 2 * constpow(BS, D - 1) + constpow(BS, D - 2)], 0);

  for (size_t z = 0; z < BS; z++) {
    for (size_t y = 0; y < BS; y++) {
      for (size_t x = 0; x < BS; x++) {
        std::cout << std::setw(5) << predicted_block[(z * BS + y) * BS + x];
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}
