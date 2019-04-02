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

template <size_t BS>
constexpr Block<DCTDATAUNIT, BS, 2> init_coefs() {
  Block<DCTDATAUNIT, BS, 2> coefs {};

  for(size_t u = 0; u < BS; ++u) {
    for(size_t x = 0; x < BS; ++x) {
      coefs[u * BS + x] = cos(((2 * x + 1) * u * M_PI ) / (2 * BS)) * sqrt(2.0 / BS) * (u == 0 ? (1 / sqrt(2)) : 1);
    }
  }

  return coefs;
}

template <size_t BS, size_t D>
struct fdct {
  template <typename IF, typename OF>
  fdct(IF &&input, OF &&output) {
    Block<DCTDATAUNIT, BS, D> tmp {};

    for (size_t slice = 0; slice < BS; slice++) {
      auto inputF = [&](size_t index) {
        return input(slice * constpow(BS, D - 1) + index);
      };

      auto outputF = [&](size_t index) -> auto & {
        return tmp[slice * constpow(BS, D - 1) + index];
      };

      fdct<BS, D - 1>(inputF, outputF);
    }

    for (size_t noodle = 0; noodle < constpow(BS, D - 1); noodle++) {
      auto inputF = [&](size_t index) {
        return tmp[index * constpow(BS, D - 1) + noodle];
      };

      auto outputF = [&](size_t index) -> auto & {
        return output(index * constpow(BS, D - 1) + noodle);
      };

      fdct<BS, 1>(inputF, outputF);
    }
  }
};


template <size_t BS>
struct fdct<BS, 1> {
  template <typename IF, typename OF>
  fdct(IF &&input, OF &&output) {
    static constexpr Block<DCTDATAUNIT, BS, 2> coefs = init_coefs<BS>();

    for (size_t u = 0; u < BS; u++) {
      for (size_t x = 0; x < BS; x++) {
        output(u) += input(x) * coefs[u * BS + x];
      }
    }
  }
};

template <size_t BS, size_t D>
struct idct {
  template <typename IF, typename OF>
  idct(IF &&input, OF &&output) {
    Block<DCTDATAUNIT, BS, D> tmp {};

    for (size_t slice = 0; slice < BS; slice++) {
      auto inputF = [&](size_t index) {
        return input(slice * constpow(BS, D - 1) + index);
      };

      auto outputF = [&](size_t index) -> auto & {
        return tmp[slice * constpow(BS, D - 1) + index];
      };

      idct<BS, D - 1>(inputF, outputF);
    }

    for (size_t noodle = 0; noodle < constpow(BS, D - 1); noodle++) {
      auto inputF = [&](size_t index) {
        return tmp[index * constpow(BS, D - 1) + noodle];
      };

      auto outputF = [&](size_t index) -> auto & {
        return output(index * constpow(BS, D - 1) + noodle);
      };

      idct<BS, 1>(inputF, outputF);
    }
  }
};

template <size_t BS>
struct idct<BS, 1> {
  template <typename IF, typename OF>
  idct(IF &&input, OF &&output) {
    static constexpr Block<DCTDATAUNIT, BS, 2> coefs = init_coefs<BS>();

    for (size_t x = 0; x < BS; x++) {
      for (size_t u = 0; u < BS; u++) {
        output(x) += input(u) * coefs[u * BS + x];
      }
    }
  }
};

#endif
