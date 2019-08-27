/******************************************************************************\
* SOUBOR: predict_test.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <predict.h>

#include <iomanip>
#include <iostream>

using namespace std;

const size_t BS = 4;
const size_t D = 3;

int main(void) {
  Block<INPUTUNIT, (2 * BS) + 2, D> test_block {};
  Block<INPUTUNIT, BS, D> predicted_block {};
  std::array<size_t, D + 1> strides {};

  for (size_t i { 0 }; i < constpow((2 * BS) + 2, D); i++) {
    test_block[i] = i;
  }

  for (size_t z = 0; z < 2 * BS + 2; z++) {
    for (size_t y = 0; y < 2 * BS + 2; y++) {
      for (size_t x = 0; x < 2 * BS + 2; x++) {
        std::cout << std::setw(5) << test_block[(z * (2 * BS + 2) + y) * (2 * BS + 2) + x];
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;

  strides[0] = 1;
  for (size_t d { 0 }; d < D; d++) {
    strides[d + 1] = strides[d] * (2 * BS + 2);
  }

  int8_t direction[3] {1, -1, -1};

  predict_angle<BS, D>(predicted_block, direction, &test_block[constpow(2 * BS + 2, 2) + constpow(2 * BS + 2, 1) + constpow(2 * BS + 2, 0)], strides.data(), false);
}
