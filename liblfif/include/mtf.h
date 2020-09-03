/**
* @file mtf.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 1. 9. 2020
* @brief
*/

#ifndef MTF_H
#define MTF_H

#include <vector>
#include <cstdint>
#include <map>

template<typename T, typename F>
void move_to_front(std::vector<T> &dictionary, T *data, size_t size, F &&update) {
  for (size_t i {}; i < size; i++) {
    auto it = std::find(std::begin(dictionary), std::end(dictionary), data[i]);

    assert(it != std::end(dictionary));

    T k = std::distance(std::begin(dictionary), it);

    data[i] = k;

    update(dictionary, k);
  }
}

template<typename T, typename F>
void move_from_front(std::vector<T> &dictionary, T *data, size_t size, F &&update) {
  for (size_t i {}; i < size; i++) {
    T k = data[i];
    data[i] = dictionary[k];

    update(dictionary, k);
  }
}

template<typename T>
void update_base(std::vector<T> &dictionary, T i) {
  auto it = std::begin(dictionary) + i;
  std::rotate(std::begin(dictionary), it, std::next(it));
}

template<typename T>
void update_m1ff(std::vector<T> &dictionary, T i) {
  auto it = std::begin(dictionary) + i;
  if (it <= std::next(std::begin(dictionary))) {
    std::rotate(std::begin(dictionary), it, std::next(it));
  }
  else {
    std::rotate(std::next(std::begin(dictionary)), it, std::next(it));
  }
}

template<typename T>
void update_m1ff2(std::vector<T> &dictionary, T i, int16_t prev[2]) {
  auto it = std::begin(dictionary) + i;

  T c = *it;

  if (it <= std::next(std::begin(dictionary))) {
    std::rotate(std::begin(dictionary), it, std::next(it));
  }
  else if (prev[0] == *std::begin(dictionary) || prev[1] == *std::begin(dictionary)) {
    std::rotate(std::next(std::begin(dictionary)), it, std::next(it));
  }
  else {
    std::rotate(std::begin(dictionary), it, std::next(it));
  }

  prev[1] = prev[0];
  prev[0] = c;
}

template<typename T>
void update_sticky(std::vector<T> &dictionary, T i, double v) {
  auto it = std::begin(dictionary) + i;

  size_t new_pos = std::round(i * v);
  std::rotate(std::begin(dictionary) + new_pos, it, std::next(it));
}

void update_k(std::vector<T> &dictionary, T i, size_t k) {
  auto it = std::begin(dictionary) + i;

  size_t new_pos = std::max<int64_t>(0, static_cast<int64_t>(i) - static_cast<int64_t>(k));
  std::rotate(std::begin(dictionary) + new_pos, it, std::next(it));
}

template<typename T>
void update_c(std::vector<T> &dictionary, T i, size_t c, std::map<T, size_t> &counts) {
  auto it = std::begin(dictionary) + i;

  if (counts[*it] >= c) {
    std::rotate(std::begin(dictionary), it, std::next(it));
  }

  counts[*it]++;
}

#endif
