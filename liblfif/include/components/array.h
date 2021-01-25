#pragma once

#include <cstddef>
#include <algorithm>

template<typename T, size_t S>
struct Array {
  T data[S];

  Array(): data{} {};

  template<size_t S2>
  Array(const Array<T, S2> &other) {
    if (S2 < S) {
      std::copy(other.data, other.data + S2, data);
      std::fill(data + S2, data + S, T {});
    }
    else {
      std::copy(other.data, other.data + S, data);
    }
  }

  Array(std::initializer_list<T> &&list): data {std::move(list)} {}

  Array(const T &value) {
    std::fill(data, data + S, value);
  }

  Array(const T *other) {
    std::copy(other, other + S, data);
  }

  template<size_t S2>
  operator Array<T, S2> &() {
    return reinterpret_cast<Array<T, S2>>(*this);
  }

  const T *begin() const {
    return data;
  }

  T *begin() {
    return data;
  }

  const T *end() const {
    return data + S;
  }

  T *end() {
    return data + S;
  }

  const T &operator[](size_t index) const {
    return data[index];
  }

  T &operator[](size_t index) {
    return data[index];
  }

  static constexpr size_t size() {
    return S;
  }
};
