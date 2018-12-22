#include "zigzag.h"

std::vector<size_t> generateZigzagTable(uint64_t size) {
  std::vector<size_t> table(size * size);

  size_t x = 0;
  size_t y = 0;
  size_t output_index = 0;
  while (true) {
    table[y * size + x] = output_index++;

    if (x < size - 1) {
      x++;
    }
    else if (y < size - 1) {
      y++;
    }
    else {
      break;
    }

    while ((x > 0) && (y < size - 1)) {
      table[y * size + x] = output_index++;
      x--;
      y++;
    }

    table[y * size + x] = output_index++;

    if (y < size - 1) {
      y++;
    }
    else if (x < size - 1) {
      x++;
    }
    else {
      break;
    }

    while ((x < size - 1) && (y > 0)) {
      table[y * size + x] = output_index++;
      x++;
      y--;
    }
  }

  return table;
}
