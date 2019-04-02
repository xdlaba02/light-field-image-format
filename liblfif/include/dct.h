/******************************************************************************\
* SOUBOR: dct.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef DCT_H
#define DCT_H

#include "constpow.h"

#include <cmath>
#include <array>

using DCTDATAUNIT = float;

constexpr Block<DCTDATAUNIT, 2> init_coefs() {
  Block<DCTDATAUNIT, 2> c {};
  for(size_t u = 0; u < BLOCK_SIZE; ++u) {
    for(size_t x = 0; x < BLOCK_SIZE; ++x) {
      c[u * BLOCK_SIZE + x] = cos(((2 * x + 1) * u * M_PI ) / (2 * BLOCK_SIZE)) * sqrt(2.0/BLOCK_SIZE) * (u == 0 ? (1 / sqrt(2)) : 1);
    }
  }
  return c;
}

template <size_t D>
struct fdct {
  template <typename IF, typename OF>
  fdct(IF &&input, OF &&output) {
    DCTDATAUNIT tmp[constpow(BLOCK_SIZE, D)] {};
    for (size_t slice = 0; slice < BLOCK_SIZE; slice++) {
      fdct<D-1>([&](size_t index) -> DCTDATAUNIT { return input(slice*constpow(BLOCK_SIZE, D-1) + index); }, [&](size_t index) -> DCTDATAUNIT & { return tmp[slice * constpow(BLOCK_SIZE, D-1) + index]; });
    }
    for (size_t noodle = 0; noodle < constpow(BLOCK_SIZE, D-1); noodle++) {
      fdct<1>([&](size_t index) -> DCTDATAUNIT { return tmp[index*constpow(BLOCK_SIZE, D-1) + noodle]; }, [&](size_t index) -> DCTDATAUNIT & { return output(index * constpow(BLOCK_SIZE, D-1) + noodle); });
    }
  }
};


template <>
struct fdct<1> {
  template <typename IF, typename OF>
  fdct(IF &&input, OF &&output) {
    static constexpr Block<DCTDATAUNIT, 2> coefs = init_coefs();

    for (size_t u = 0; u < BLOCK_SIZE; u++) {
      for (size_t x = 0; x < BLOCK_SIZE; x++) {
        output(u) += input(x) * coefs[u*BLOCK_SIZE+x];
      }
    }
  }
};

template <size_t D>
struct idct {
  template <typename IF, typename OF>
  idct(IF &&input, OF &&output) {
    DCTDATAUNIT tmp[constpow(BLOCK_SIZE, D)] {};
    for (size_t slice = 0; slice < BLOCK_SIZE; slice++) {
      idct<D-1>([&](size_t index) -> DCTDATAUNIT { return input(slice*constpow(BLOCK_SIZE, D-1) + index); }, [&](size_t index) -> DCTDATAUNIT & { return tmp[slice*constpow(BLOCK_SIZE, D-1) + index]; });
    }
    for (size_t noodle = 0; noodle < constpow(BLOCK_SIZE, D-1); noodle++) {
      idct<1>([&](size_t index) -> DCTDATAUNIT { return tmp[index*constpow(BLOCK_SIZE, D-1) + noodle]; }, [&](size_t index) -> DCTDATAUNIT & { return output(index*constpow(BLOCK_SIZE, D-1) + noodle); });
    }
  }
};

template <>
struct idct<1> {
  template <typename IF, typename OF>
  idct(IF &&input, OF &&output) {
    static constexpr Block<DCTDATAUNIT, 2> coefs = init_coefs();
    
    for (size_t x = 0; x < BLOCK_SIZE; x++) {
      for (size_t u = 0; u < BLOCK_SIZE; u++) {
        output(x) += input(u) * coefs[u*BLOCK_SIZE+x];
      }
    }
  }
};

#endif
