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

  for (size_t d { 0 }; d < D; d++) {
    Block<INPUTUNIT, (2 * BS) + 2, D - 1> slice {};

    for (size_t i { 0 }; i < constpow((2 * BS) + 2, D - 1); i++) {
      slice[i] = d + 1;
    }

    putSlice<BS * 2 + 2, D, INPUTUNIT>(test_block, slice, d, 0);
  }

  for (size_t z = 0; z < 2 * BS + 2; z++) {
    for (size_t y = 0; y < 2 * BS + 2; y++) {
      for (size_t x = 0; x < 2 * BS + 2; x++) {
        std::cout << std::setw(3) << test_block[(z * (2*BS + 2) + y) * (2 * BS + 2) + x];
      }
      std::cout << '\n';
    }
    std::cout << '\n';
  }
  std::cout << '\n';

  strides[0] = 1;
  for (size_t d { 0 }; d < D; d++) {
    strides[d + 1] = strides[d] * (2 * BS + 2);
  }

  int8_t direction[D] {1, 0, 0};

  int64_t offset {};
  for (size_t d { 0 }; d < D; d++) {
    offset += constpow(2 * BS + 2, d);
  }

  predict_direction<BS, D>(predicted_block, direction, &test_block[offset], strides.data());

  std::cerr << '\n';

  for (size_t z = 0; z < BS; z++) {
    for (size_t y = 0; y < BS; y++) {
      for (size_t x = 0; x < BS; x++) {
        std::cout << std::setw(4) << std::fixed << std::setprecision(1) << predicted_block[(z * BS + y) * BS + x];
      }
      std::cout << '\n';
    }
    std::cout << '\n';
  }
  std::cout << '\n';

}
