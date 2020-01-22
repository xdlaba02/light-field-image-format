/**
* @file zigzag.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief The algorithm for generating zig-zag matrices.
*/

#ifndef ZIGZAG_H
#define ZIGZAG_H

#include "lfiftypes.h"

#include <cstdint>
#include <vector>
#include <array>
#include <numeric>

/**
 * @brief Function which shuffles the block.
 * @param table A block which shall be shuffled.
 */
template <size_t D>
DynamicBlock<size_t, D> shuffle(const DynamicBlock<size_t, D> &block, const size_t shuffle_idxs[D]) {
  std::array<size_t, D> shuffled_size {};

  for (size_t i {}; i < D; i++) {
     shuffled_size[i] = block.size(shuffle_idxs[i]);
  }

  DynamicBlock<size_t, D> shuffled_block(shuffled_size);

  iterate_dimensions<D>(block.size(), [&](const std::array<size_t, D> &pos) {
    std::array<size_t, D> shuffled_pos {};

    for (size_t i {}; i < D; i++) {
      shuffled_pos[i] = pos[shuffle_idxs[i]];
    }

    shuffled_block[shuffled_pos] = block[pos];
  });

  return shuffled_block;
}

template <size_t D>
void zigzagCore(DynamicBlock<size_t, D> &block, std::array<size_t, D> &pos, size_t &index, size_t depth) {
  if (depth > 1) {
    auto move = [&]() {
      for (size_t i { 2 }; i <= depth; i++) {
        if ((pos[depth - i] > 0) && (pos[depth - 1] < block.size()[depth - 1] - 1)) {
          pos[depth - i]--;
          pos[depth - 1]++;
          return true;
        }
      }
      return false;
    };

    do {
      zigzagCore<D>(block, pos, index, depth - 1);
    } while (move());

    size_t shuffle_idxs[D] {};
    for (size_t i { 1 }; i <= depth; i++) {
      shuffle_idxs[i % depth] = i - 1;
    }

    for (size_t i { depth }; i < D; i++) {
      shuffle_idxs[i] = i;
    }

    std::array<size_t, D> shuffled_pos {};
    for (size_t i {}; i < D; i++) {
      shuffled_pos[i] = pos[shuffle_idxs[i]];
    }

    pos   = shuffled_pos;
    block = shuffle<D>(block, shuffle_idxs);
  }
  else {
    block[pos] = index++;
  }
}

template <size_t D>
DynamicBlock<size_t, D> zigzagTable(const std::array<size_t, D> &size) {
  DynamicBlock<size_t, D> block(size);
  std::array<size_t, D>   pos   {};
  size_t                  index {};

  auto move = [&]() {
    for (size_t i { 0 }; i < D; i++) {
      if (pos[i] < block.size()[i] - 1) {
        pos[i]++;
        return true;
      }
    }
    return false;
  };

  do {
    zigzagCore<D>(block, pos, index, D);
  } while (move());

  auto correct_shuffle = [&](const size_t shuffle_idxs[D]) {
    for (size_t i { 0 }; i < D; i++) {
      if (size[i] != block.size()[shuffle_idxs[i]]) {
        return false;
      }
    }
    return true;
  };

  size_t shuffle_idxs[D] {};
  for (size_t i { 0 }; i < D; i++) {
    shuffle_idxs[i] = i;
  }

  while (!correct_shuffle(shuffle_idxs)) {
    std::next_permutation(std::begin(shuffle_idxs), std::end(shuffle_idxs));
  }

  return shuffle<D>(block, shuffle_idxs);
}

[[deprecated]]
DynamicBlock<size_t, 2> generateZigzagTable2D(const std::array<size_t, 2> &size) {
  DynamicBlock<size_t, 2> block(size);
  size_t                  index {};
  std::array<size_t, 2>   pos   {};

  while (true) {
    while (true) {
      block[pos] = index++;
      if (pos[0] > 0 && pos[1] < block.size()[1] - 1) {
        pos[0]--;
        pos[1]++;
      }
      else {
        break;
      }
    }

    size_t shuffle_idxs[2] {};
    shuffle_idxs[0] = 1;
    shuffle_idxs[1] = 0;

    std::array<size_t, 2> shuffled_pos   {};
    for (size_t i {}; i < 2; i++) {
      shuffled_pos[i] = pos[shuffle_idxs[i]];
    }
    pos   = shuffled_pos;
    block = shuffle<2>(block, shuffle_idxs);

    if (pos[0] < block.size()[0] - 1) {
      pos[0]++;
    }
    else if (pos[1] < block.size()[1] - 1){
      pos[1]++;
    }
    else {
      break;
    }
  }

  auto correct_shuffle = [&](const size_t shuffle_idxs[2]) {
    for (size_t i { 0 }; i < 2; i++) {
      if (size[i] != block.size()[shuffle_idxs[i]]) {
        return false;
      }
    }
    return true;
  };

  size_t shuffle_idxs[2] {};
  for (size_t i { 0 }; i < 2; i++) {
    shuffle_idxs[i] = i;
  }

  while (!correct_shuffle(shuffle_idxs)) {
    std::next_permutation(std::begin(shuffle_idxs), std::end(shuffle_idxs));
  }

  return shuffle<2>(block, shuffle_idxs);
}

[[deprecated]]
DynamicBlock<size_t, 3> generateZigzagTable3D(const std::array<size_t, 3> &size) {
  DynamicBlock<size_t, 3> block(size);
  size_t                  index {};
  std::array<size_t, 3>   pos   {};

  while (true) {
    while (true) {
      while (true) {
        block[pos] = index++;
        if (pos[0] > 0 && pos[1] < block.size()[1] - 1) {
          pos[0]--;
          pos[1]++;
        }
        else {
          break;
        }
      }

      size_t shuffle_idxs[3] {};
      shuffle_idxs[0] = 1;
      shuffle_idxs[1] = 0;
      shuffle_idxs[2] = 2;

      std::array<size_t, 3> shuffled_pos {};
      for (size_t i {}; i < 3; i++) {
        shuffled_pos[i] = pos[shuffle_idxs[i]];
      }
      pos   = shuffled_pos;
      block = shuffle<3>(block, shuffle_idxs);

      if (pos[1] > 0 && pos[2] < block.size()[2] - 1) {
        pos[1]--;
        pos[2]++;
      }
      else if (pos[0] > 0 && pos[2] < block.size()[2] - 1) {
        pos[0]--;
        pos[2]++;
      }
      else {
        break;
      }
    }

    size_t shuffle_idxs[3] {};
    shuffle_idxs[0] = 2;
    shuffle_idxs[1] = 0;
    shuffle_idxs[2] = 1;

    std::array<size_t, 3> shuffled_pos {};
    for (size_t i {}; i < 3; i++) {
      shuffled_pos[i] = pos[shuffle_idxs[i]];
    }
    pos   = shuffled_pos;
    block = shuffle<3>(block, shuffle_idxs);

    if (pos[0] < block.size()[0] - 1) {
      pos[0]++;
    }
    else if (pos[1] < block.size()[1] - 1) {
      pos[1]++;
    }
    else if (pos[2] < block.size()[2] - 1){
      pos[2]++;
    }
    else {
      break;
    }
  }

  auto correct_shuffle = [&](const size_t shuffle_idxs[3]) {
    for (size_t i { 0 }; i < 3; i++) {
      if (size[i] != block.size()[shuffle_idxs[i]]) {
        return false;
      }
    }
    return true;
  };

  size_t shuffle_idxs[3] {};
  for (size_t i { 0 }; i < 3; i++) {
    shuffle_idxs[i] = i;
  }

  while (!correct_shuffle(shuffle_idxs)) {
    std::next_permutation(std::begin(shuffle_idxs), std::end(shuffle_idxs));
  }

  return shuffle<3>(block, shuffle_idxs);
}

[[deprecated]]
DynamicBlock<size_t, 4> generateZigzagTable4D(const std::array<size_t, 4> &size) {
  DynamicBlock<size_t, 4> block(size);
  size_t                  index {};
  std::array<size_t, 4>   pos   {};

  while (true) {
    while (true) {
      while (true) {
        while (true) {
          block[pos] = index++;
          if (pos[0] > 0 && pos[1] < block.size()[1] - 1) {
            pos[0]--;
            pos[1]++;
          }
          else {
            break;
          }
        }

        size_t shuffle_idxs[4] {};
        shuffle_idxs[0] = 1;
        shuffle_idxs[1] = 0;
        shuffle_idxs[2] = 2;
        shuffle_idxs[3] = 3;

        std::array<size_t, 4> shuffled_pos {};
        for (size_t i {}; i < 4; i++) {
          shuffled_pos[i] = pos[shuffle_idxs[i]];
        }
        pos   = shuffled_pos;
        block = shuffle<4>(block, shuffle_idxs);

        if (pos[1] > 0 && pos[2] < block.size()[2] - 1) {
          pos[1]--;
          pos[2]++;
        }
        else if (pos[0] > 0 && pos[2] < block.size()[2] - 1) {
          pos[0]--;
          pos[2]++;
        }
        else {
          break;
        }
      }

      size_t shuffle_idxs[4] {};
      shuffle_idxs[0] = 2;
      shuffle_idxs[1] = 0;
      shuffle_idxs[2] = 1;
      shuffle_idxs[3] = 3;

      std::array<size_t, 4> shuffled_pos {};
      for (size_t i {}; i < 4; i++) {
        shuffled_pos[i] = pos[shuffle_idxs[i]];
      }
      pos   = shuffled_pos;
      block = shuffle<4>(block, shuffle_idxs);

      if (pos[2] > 0 && pos[3] < block.size()[3] - 1) {
        pos[2]--;
        pos[3]++;
      }
      else if (pos[1] > 0 && pos[3] < block.size()[3] - 1) {
        pos[1]--;
        pos[3]++;
      }
      else if (pos[0] > 0 && pos[3] < block.size()[3] - 1) {
        pos[0]--;
        pos[3]++;
      }
      else {
        break;
      }
    }

    size_t shuffle_idxs[4] {};
    shuffle_idxs[0] = 3;
    shuffle_idxs[1] = 0;
    shuffle_idxs[2] = 1;
    shuffle_idxs[3] = 2;

    std::array<size_t, 4> shuffled_pos {};
    for (size_t i {}; i < 4; i++) {
      shuffled_pos[i] = pos[shuffle_idxs[i]];
    }
    pos   = shuffled_pos;
    block = shuffle<4>(block, shuffle_idxs);

    if (pos[0] < block.size()[0] - 1) {
      pos[0]++;
    }
    else if (pos[1] < block.size()[1] - 1) {
      pos[1]++;
    }
    else if (pos[2] < block.size()[2] - 1){
      pos[2]++;
    }
    else if (pos[3] < block.size()[3] - 1){
      pos[3]++;
    }
    else {
      break;
    }
  }

  auto correct_shuffle = [&](const size_t shuffle_idxs[4]) {
    for (size_t i { 0 }; i < 4; i++) {
      if (size[i] != block.size()[shuffle_idxs[i]]) {
        return false;
      }
    }
    return true;
  };

  size_t shuffle_idxs[4] {};
  for (size_t i { 0 }; i < 4; i++) {
    shuffle_idxs[i] = i;
  }

  while (!correct_shuffle(shuffle_idxs)) {
    std::next_permutation(std::begin(shuffle_idxs), std::end(shuffle_idxs));
  }

  return shuffle<4>(block, shuffle_idxs);
}

#endif
