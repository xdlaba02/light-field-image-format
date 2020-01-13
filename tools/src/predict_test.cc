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
const int8_t direction[D] {1, 1};

int main(void) {
  Block<INPUTUNIT, (2 * BS) + 1, D> input_block     {};
  Block<INPUTUNIT, BS, D>           predicted_block {};
  std::array<size_t, D>     block_offsets {};
  std::array<size_t, D>     input_dims    {};
  std::array<size_t, D + 1> input_strides {};

  for (size_t d { 0 }; d < D; d++) {
    Block<INPUTUNIT, (2 * BS) + 1, D - 1> slice {};

    for (size_t i { 0 }; i < constpow((2 * BS) + 1, D - 1); i++) {
      slice[i] = i + 1;
    }

    putSlice<BS * 2 + 1, D, INPUTUNIT>(input_block, slice, d, 0);
  }

  for (size_t y = 0; y < 2 * BS + 1; y++) {
    for (size_t x = 0; x < 2 * BS + 1; x++) {
      std::cout << std::setw(3) << input_block[y * (2 * BS + 1) + x];
    }
    std::cout << '\n';
  }
  std::cout << '\n';

  input_strides[0] = 1;
  for (size_t d { 0 }; d < D; d++) {
    block_offsets[d] = 1;
    input_dims[d] = BS * 2 + 1;
    input_strides[d + 1] = input_strides[d] * input_dims[d];
  }

  int64_t offset {};
  for (size_t d { 0 }; d < D; d++) {
    offset += constpow(2 * BS + 1, d);
  }

  auto inputF = [&](std::array<int64_t, D> &block_pos) {
    std::cerr << "[ ";
    for (size_t i { 0 }; i < D; i++) {
      std::cerr << block_pos[i] << " ";
    }
    std::cerr << "] -> [ ";

    for (size_t i { 1 }; i < D; i++) {
      if (block_pos[i] >= static_cast<int64_t>(BS)) {
        block_pos[i] = BS - 1;
      }
    }

    for (size_t i { 0 }; i < D; i++) {
      std::cerr << block_pos[i] << " ";
    }
    std::cerr << "] -> [ ";

    size_t img_idx {};
    std::array<int64_t, D> img_pos {};
    for (size_t i { 0 }; i < D; i++) {
      img_pos[i] = block_offsets[i] + block_pos[i];

      if (img_pos[0] < 1) {
        img_pos[0] = 1;
      }

      if (img_pos[i] < 0) {
        img_pos[i] = 0;
      }
      else if (img_pos[i] >= static_cast<int64_t>(input_dims[i])) {
        img_pos[i] = input_dims[i] - 1;
      }

      img_idx += img_pos[i] * input_strides[i];
    }

    for (size_t i { 0 }; i < D; i++) {
      std::cerr << img_pos[i] << " ";
    }
    std::cerr << "] = " << input_block[img_idx] << "\n";

    return input_block[img_idx];
  };

  predict_direction<BS, D>(predicted_block, direction, inputF);

  for (size_t y = 0; y < BS; y++) {
    for (size_t x = 0; x < BS; x++) {
      std::cout << std::setw(4) << std::fixed << std::setprecision(1) << predicted_block[y * BS + x];
    }
    std::cout << '\n';
  }
  std::cout << '\n';
}
