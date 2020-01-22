/******************************************************************************\
* SOUBOR: dynamix_test.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <zigzag.h>

#include <iomanip>
#include <iostream>

using namespace std;

const size_t D = 4;

int main() {
  size_t BS[D] { 3, 4, 5, 6 };

  DynamicBlock<size_t, D> block = zigzagTable<D>(BS);

  iterate_dimensions<D>(block.size(), [&](const std::array<size_t, D> &pos) {
    std::cout << std::setw(4) << block[pos];

    for (size_t i {}; i < D; i++) {
      if (pos[i] == block.size()[i] - 1) {
        std::cout << '\n';
      }
      else {
        break;
      }
    }
  });

  for (size_t h {}; h < block.size()[3]; h++) {
    for (size_t y {}; y < block.size()[1]; y++) {
      for (size_t w {}; w < block.size()[2]; w++) {
        for (size_t x {}; x < block.size()[0]; x++) {
          std::cout << std::setw(5) << block[{x, y, w, h}];
        }
        std::cout << " ";
      }
      std::cout << '\n';
    }
    std::cout << '\n';
  }
  std::cout << '\n';
}
