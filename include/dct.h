#ifndef DCT_H
#define DCT_H

#include "constmath.h"

#include <cstdint>
#include <cmath>
#include <functional>

using namespace std;

constexpr array<float, 64> init_coefs() {
  array<float, 64> c {};
  for(uint64_t u = 0; u < 8; ++u) {
    for(uint64_t x = 0; x < 8; ++x) {
      c[u*8+x] = cos(((2 * x + 1) * u * M_PI ) / 16) * sqrt(0.25) * (u == 0 ? (1 / sqrt(2)) : 1);
    }
  }
  return c;
}

constexpr array<float, 64> coefs = init_coefs();

template <uint8_t D>
struct fdct {
  template <typename IF, typename OF>
  fdct(IF &&input, OF &&output) {
    float tmp[constpow(8, D)] {};
    for (uint8_t slice = 0; slice < 8; slice++) {
      fdct<D-1>([&](uint64_t index) -> float { return input(slice*constpow(8, D-1) + index); }, [&](uint64_t index) -> float & { return tmp[slice*constpow(8, D-1) + index]; });
    }
    for (uint64_t noodle = 0; noodle < constpow(8, D-1); noodle++) {
      fdct<1>([&](uint8_t index) -> float { return tmp[index*constpow(8, D-1) + noodle]; }, [&](uint8_t index) -> float & { return output(index*constpow(8, D-1) + noodle); });
    }
  }
};


template <>
struct fdct<1> {
  template <typename IF, typename OF>
  fdct(IF &&input, OF &&output) {
    for (uint8_t u = 0; u < 8; u++) {
      for (uint8_t x = 0; x < 8; x++) {
        output(u) += input(x) * coefs[u*8+x];
      }
    }
  }
};

template <uint8_t D>
struct idct {
  template <typename IF, typename OF>
  idct(IF &&input, OF &&output) {
    float tmp[constpow(8, D)] {};
    for (uint8_t slice = 0; slice < 8; slice++) {
      idct<D-1>([&](uint64_t index) -> float { return input(slice*constpow(8, D-1) + index); }, [&](uint64_t index) -> float & { return tmp[slice*constpow(8, D-1) + index]; });
    }
    for (uint64_t noodle = 0; noodle < constpow(8, D-1); noodle++) {
      idct<1>([&](uint8_t index) -> float { return tmp[index*constpow(8, D-1) + noodle]; }, [&](uint8_t index) -> float & { return output(index*constpow(8, D-1) + noodle); });
    }
  }
};

template <>
struct idct<1> {
  template <typename IF, typename OF>
  idct(IF &&input, OF &&output) {
    for (uint8_t x = 0; x < 8; x++) {
      for (uint8_t u = 0; u < 8; u++) {
        output(x) += input(u) * coefs[u*8+x];
      }
    }
  }
};

#endif
