/******************************************************************************\
* SOUBOR: predict_test.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <predict.h>

#include <iomanip>
#include <iostream>

using namespace std;

const size_t BS = 4;
const size_t D = 2;

int main(void) {
  iterate_cube<BS, D>([&](const std::array<size_t, D> &pos) {
    std::cerr << "[ ";
    for (size_t i = 0; i < D; i++) {
      std::cerr << pos[i] << " ";
    }
    std::cerr << "] " << get_index<BS, D>(pos) << "\n";
  });

  Block<INPUTUNIT, (2 * BS) + 1, D> test_block {};
  Block<INPUTUNIT, BS, D> predicted_block {};
  std::array<size_t, D + 1> strides {};

  bool neighbours_prev[D] {};
  bool neighbours_next[D] {};

  for (size_t d { 0 }; d < D; d++) {
    Block<INPUTUNIT, (2 * BS) + 1, D - 1> slice {};

    for (size_t i { 0 }; i < constpow((2 * BS) + 1, D - 1); i++) {
      slice[i] = i;
    }

    putSlice<BS * 2 + 1, D, INPUTUNIT>(test_block, slice, d, 0);

    neighbours_prev[d] = true;
    neighbours_next[d] = false;
  }

  neighbours_next[0] = true;

    for (size_t y = 0; y < 2 * BS + 1; y++) {
      for (size_t x = 0; x < 2 * BS + 1; x++) {
        std::cout << std::setw(3) << test_block[y * (2 * BS + 1) + x];
      }
      std::cout << '\n';
    }
    std::cout << '\n';

  strides[0] = 1;
  for (size_t d { 0 }; d < D; d++) {
    strides[d + 1] = strides[d] * (2 * BS + 1);
  }

  int8_t direction[D] {1, 1};

  int64_t offset {};
  for (size_t d { 0 }; d < D; d++) {
    offset += constpow(2 * BS + 1, d);
  }

  auto inputF = [&](std::array<int64_t, D> position) {
    int64_t src_idx {};

    std::cerr << '(';
    for (size_t d { 0 }; d < D; d++) {
      std::cerr << position[d] << ',';
    }
    std::cerr << ") -> ";

    std::cerr << '(';
    for (size_t d { 0 }; d < D; d++) {

      if ((position[d] < 0) && !neighbours_prev[d]) {
        position[d] = 0;
      }
      else if ((position[d] > static_cast<int64_t>(BS - 1)) && !neighbours_next[d]) {
        position[d] = BS - 1;
      }

      std::cerr << position[d] << ',';

      src_idx += position[d] * strides[d];
    }
    std::cerr << ") " << test_block[offset + src_idx] << "\n";

    return test_block[offset + src_idx];
  };

  predict_direction<BS, D>(predicted_block, direction, inputF);

  std::cerr << '\n';

    for (size_t y = 0; y < BS; y++) {
      for (size_t x = 0; x < BS; x++) {
        std::cout << std::setw(4) << std::fixed << std::setprecision(1) << predicted_block[y * BS + x];
      }
      std::cout << '\n';
    }
    std::cout << '\n';
}
