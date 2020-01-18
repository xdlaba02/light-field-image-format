/******************************************************************************\
* SOUBOR: predict_test.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <predict.h>
#include <meta.h>

#include <iomanip>
#include <iostream>

using namespace std;

const size_t D = 2;
const std::array<size_t, D> BS = {2, 6};
const std::array<size_t, D> input_size = {10, 10};
const std::array<int8_t, D> direction = {1, 1};

int main(void) {
  DynamicBlock<INPUTUNIT, D> input_block(input_size.data());
  DynamicBlock<INPUTUNIT, D> predicted_block(BS.data());
  std::array<size_t, D>      block_offsets   {};

  std::fill(std::begin(block_offsets), std::end(block_offsets), 1);

  for (size_t i { 0 }; i < D; i++) {
    std::array<size_t, D - 1> slice_size {};

    for (size_t j {}; j < D - 1; j++){
      size_t idx = j < i ? j : j + 1;
      slice_size[j] = input_size[idx];
    }

    iterate_dimensions<D - 1>(slice_size, [&](const std::array<size_t, D - 1> &pos) {
      std::array<size_t, D> input_block_pos {};

      for (size_t j {}; j < D - 1; j++){
        size_t idx = j < i ? j : j + 1;
        input_block_pos[idx] = pos[j];
      }

      input_block[input_block_pos] = make_index<D>(input_size.data(), input_block_pos);
    });
  }

  iterate_dimensions<D>(input_size, [&](const std::array<size_t, D> &pos) {
    std::cout << std::setw(4) << input_block[pos];

    for (size_t i {}; i < D - 1; i++) {
      if (pos[i] == input_size[i] - 1) {
        std::cout << '\n';
      }
    }
  });

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

    for (size_t i { 1 }; i < D; i++) {
      if (block_pos[i] >= static_cast<int64_t>(BS[i])) {
        block_pos[i] = BS[i] - 1;
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
      else if (img_pos[i] >= static_cast<int64_t>(input_size[i])) {
        img_pos[i] = input_size[i] - 1;
      }
    }

    size_t img_idx {};
    for (size_t i { 1 }; i <= D; i++) {
      img_idx *= input_size[D - i];
      img_idx += img_pos[D - i];
    }

    for (size_t i { 0 }; i < D; i++) {
      std::cerr << img_pos[i] << " ";
    }
    std::cerr << "] = " << input_block[img_idx] << "\n";

    return input_block[img_idx];
  };

  predict_direction<D>(predicted_block, direction.data(), inputF);

  iterate_dimensions<D>(BS, [&](const std::array<size_t, D> &pos) {
    std::cout << std::setw(6) << std::fixed << std::setprecision(2) << predicted_block[pos];

    for (size_t i {}; i < D - 1; i++) {
      if (pos[i] == BS[i] - 1) {
        std::cout << '\n';
      }
    }
  });
}
