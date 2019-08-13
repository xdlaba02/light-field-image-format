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
  Block<INPUTUNIT, BS, D> test_block {};

  for (size_t i { 0 }; i < constpow(BS, D); i++) {
    test_block[i] = i;
  }

  Block<INPUTUNIT, BS, D - 1> slices[D] {};

  for (size_t d { 0 }; d < D; d++) {
    slices[d] = getSlice<BS, D>(test_block, d, BS - 1);
  }

  Block<INPUTUNIT, BS, D> output = predictDiag<BS, D>( { &slices[0], &slices[1], &slices[2] } );

  for (size_t y = 0; y < BS; y++) {
    for (size_t x = 0; x < BS; x++) {
      std::cout << std::setw(4) << getSlice<BS, D>(test_block, 0, BS - 1)[y * BS + x];
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;

  for (size_t y = 0; y < BS; y++) {
    for (size_t x = 0; x < BS; x++) {
      std::cout << std::setw(4) << getSlice<BS, D>(test_block, 1, BS - 1)[y * BS + x];
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;

  for (size_t y = 0; y < BS; y++) {
    for (size_t x = 0; x < BS; x++) {
      std::cout << std::setw(4) << getSlice<BS, D>(test_block, 2, BS - 1)[y * BS + x];
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;


  for (size_t z = 0; z < BS; z++) {
    for (size_t y = 0; y < BS; y++) {
      for (size_t x = 0; x < BS; x++) {
        std::cout << std::setw(4) << output[(z * BS + y) * BS + x];
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
}
