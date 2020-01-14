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
const int8_t direction[D] {2, 2};

int main(void) {
  Block<INPUTUNIT, (2 * BS) + 1, D> input_block     {};
  Block<INPUTUNIT, BS, D>           predicted_block {};
  std::array<size_t, D>             block_offsets   {};
  std::array<size_t, D>             input_dims      {};
  std::array<size_t, D + 1>         input_strides   {};

  for (size_t d { 0 }; d < D; d++) {
    Block<INPUTUNIT, (2 * BS) + 1, D - 1> slice {};

    for (size_t i { 0 }; i < constpow((2 * BS) + 1, D - 1); i++) {
      slice[i] = constpow((2 * BS) + 1, D - 1) * d + i;
    }

    putSlice<BS * 2 + 1, D, INPUTUNIT>(input_block, slice, d, 0);
  }

  for (size_t z = 0; z < 1; z++) {
    for (size_t y = 0; y < 2 * BS + 1; y++) {
      for (size_t x = 0; x < 2 * BS + 1; x++) {
        std::cout << std::setw(4) << input_block[(z * (2 * BS + 1) + y) * (2 * BS + 1) + x];
      }
      std::cout << '\n';
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

    bool is_available[D] {};

    for (size_t i { 0 }; i < D; i++) {
      is_available[i] = true;
    }

    is_available[0] = false;
    //is_available[1] = false;


    for (size_t i { 1 }; i < D; i++) {
      if (block_pos[i] >= static_cast<int64_t>(BS)) {
        block_pos[i] = BS - 1;
        std::cerr << "NOOT ";
      }
    }

    for (size_t i { 0 }; i < D; i++) {
      if (!is_available[i] && block_pos[i] < 0) {
        block_pos[i] = 0;
      }
      std::cerr << block_pos[i] << " ";
    }
    std::cerr << "] -> [ ";

    int64_t max_pos {};
    for (size_t i = 0; i < D; i++) {
      if (is_available[i] && (block_pos[i] + 1) > max_pos) {
        max_pos = block_pos[i] + 1;
      }
    }

    int64_t min_pos { max_pos }; // asi bude lepsi int64t maximum

    for (size_t i = 0; i < D; i++) {
      if (is_available[i] && (block_pos[i] + 1) < min_pos) {
        min_pos = block_pos[i] + 1;
      }
    }

    for (size_t i = 0; i < D; i++) {
      if (is_available[i]) {
        block_pos[i] -= min_pos;
      }
    }

    for (size_t i { 0 }; i < D; i++) {
      std::cerr << block_pos[i] << " ";
    }
    std::cerr << "] -> [ ";

    std::array<int64_t, D> img_pos {};
    for (size_t i { 0 }; i < D; i++) {
      img_pos[i] = block_offsets[i] + block_pos[i];

      if (img_pos[i] < 0) {
        img_pos[i] = 0;
      }
      else if (img_pos[i] >= static_cast<int64_t>(input_dims[i])) {
        img_pos[i] = input_dims[i] - 1;
      }
    }

    size_t img_idx {};
    for (size_t i { 0 }; i < D; i++) {
      img_idx += img_pos[i] * input_strides[i];
    }

    for (size_t i { 0 }; i < D; i++) {
      std::cerr << img_pos[i] << " ";
    }
    std::cerr << "] = " << input_block[img_idx] << "\n";

    return input_block[img_idx];
  };

  predict_direction<BS, D>(predicted_block, direction, inputF);

  for (size_t z = 0; z < 1; z++) {
    for (size_t y = 0; y < BS; y++) {
      for (size_t x = 0; x < BS; x++) {
        std::cout << std::setw(6) << std::fixed << std::setprecision(2) << predicted_block[(z * BS + y) * BS + x];
      }
      std::cout << '\n';
    }
    std::cout << '\n';
  }
  std::cout << '\n';
}
