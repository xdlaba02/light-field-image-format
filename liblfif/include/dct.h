/******************************************************************************\
* SOUBOR: dct.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef DCT_H
#define DCT_H

#include "constpow.h"

#include <cmath>
#include <array>

using DCTDATAUNIT = double;

constexpr std::array<DCTDATAUNIT, 64> init_coefs() {
  std::array<DCTDATAUNIT, 64> c {};
  for(size_t u = 0; u < 8; ++u) {
    for(size_t x = 0; x < 8; ++x) {
      c[u*8+x] = cos(((2 * x + 1) * u * M_PI ) / 16) * sqrt(0.25) * (u == 0 ? (1 / sqrt(2)) : 1);
    }
  }
  return c;
}

constexpr std::array<DCTDATAUNIT, 64> coefs = init_coefs();

template <size_t D>
struct fdct {
  template <typename IF, typename OF>
  fdct(IF &&input, OF &&output) {
    DCTDATAUNIT tmp[constpow(8, D)] {};
    for (size_t slice = 0; slice < 8; slice++) {
      fdct<D-1>([&](size_t index) -> DCTDATAUNIT { return input(slice*constpow(8, D-1) + index); }, [&](size_t index) -> DCTDATAUNIT & { return tmp[slice*constpow(8, D-1) + index]; });
    }
    for (size_t noodle = 0; noodle < constpow(8, D-1); noodle++) {
      fdct<1>([&](size_t index) -> DCTDATAUNIT { return tmp[index*constpow(8, D-1) + noodle]; }, [&](size_t index) -> DCTDATAUNIT & { return output(index*constpow(8, D-1) + noodle); });
    }
  }
};


template <>
struct fdct<1> {
  template <typename IF, typename OF>
  fdct(IF &&input, OF &&output) {
    for (size_t u = 0; u < 8; u++) {
      for (size_t x = 0; x < 8; x++) {
        output(u) += input(x) * coefs[u*8+x];
      }
    }
  }
};

template <size_t D>
struct idct {
  template <typename IF, typename OF>
  idct(IF &&input, OF &&output) {
    DCTDATAUNIT tmp[constpow(8, D)] {};
    for (size_t slice = 0; slice < 8; slice++) {
      idct<D-1>([&](size_t index) -> DCTDATAUNIT { return input(slice*constpow(8, D-1) + index); }, [&](size_t index) -> DCTDATAUNIT & { return tmp[slice*constpow(8, D-1) + index]; });
    }
    for (size_t noodle = 0; noodle < constpow(8, D-1); noodle++) {
      idct<1>([&](size_t index) -> DCTDATAUNIT { return tmp[index*constpow(8, D-1) + noodle]; }, [&](size_t index) -> DCTDATAUNIT & { return output(index*constpow(8, D-1) + noodle); });
    }
  }
};

template <>
struct idct<1> {
  template <typename IF, typename OF>
  idct(IF &&input, OF &&output) {
    for (size_t x = 0; x < 8; x++) {
      for (size_t u = 0; u < 8; u++) {
        output(x) += input(u) * coefs[u*8+x];
      }
    }
  }
};

#endif
