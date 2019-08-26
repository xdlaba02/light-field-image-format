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
  Block<INPUTUNIT, BS + 1, D> test_block {};
  Block<INPUTUNIT, BS, D> predicted_block {};
  std::array<size_t, D> dims {};

  for (size_t i { 0 }; i < constpow(BS + 1, D); i++) {
    test_block[i] = i;
  }

  for (size_t z = 0; z < BS + 1; z++) {
    for (size_t y = 0; y < BS + 1; y++) {
      for (size_t x = 0; x < BS + 1; x++) {
        std::cout << std::setw(5) << test_block[(z * (BS + 1) + y) * (BS + 1) + x];
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;

  dims.fill(BS + 1);

  int8_t direction[3] {0, 0, 1};

  predict_angle<BS, D>(predicted_block, direction, &test_block[constpow(BS + 1, 2) + constpow(BS + 1, 1) + constpow(BS + 1, 0)], dims.data(), false);
}
