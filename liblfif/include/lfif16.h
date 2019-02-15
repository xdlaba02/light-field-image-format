/******************************************************************************\
* SOUBOR: lfif16.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF16_H
#define LFIF16_H

#include <cstdint>
#include <vector>
#include <array>
#include <cmath>

using namespace std;

struct RunLengthPair {
  size_t zeroes;
  int32_t amplitude;
};

struct HuffmanTable {
  vector<uint8_t> counts;
  vector<uint8_t> symbols;
};

int LFIFCompress16(const uint16_t *rgb_data, const uint64_t img_dims[2], uint8_t quality, const char *output_file_name);
int LFIFDecompress16(const char *input_file_name, vector<uint16_t> &rgb_data, uint64_t img_dims[2]);

constexpr array<double, 64> init_coefs() {
  array<double, 64> c {};
  for(size_t u = 0; u < 8; ++u) {
    for(size_t x = 0; x < 8; ++x) {
      c[u*8+x] = cos(((2 * x + 1) * u * M_PI ) / 16) * sqrt(0.25) * (u == 0 ? (1 / sqrt(2)) : 1);
    }
  }
  return c;
}

constexpr array<double, 64> coefs = init_coefs();

template <size_t D>
struct fdct {
  template <typename IF, typename OF>
  fdct(IF &&input, OF &&output) {
    double tmp[static_cast<unsigned>(pow(8, D))] {};
    for (size_t slice = 0; slice < 8; slice++) {
      fdct<D-1>([&](size_t index) -> double { return input(slice*static_cast<unsigned>(pow(8, D-1)) + index); }, [&](size_t index) -> double & { return tmp[slice*static_cast<unsigned>(pow(8, D-1)) + index]; });
    }
    for (size_t noodle = 0; noodle < static_cast<unsigned>(pow(8, D-1)); noodle++) {
      fdct<1>([&](size_t index) -> double { return tmp[index*static_cast<unsigned>(pow(8, D-1)) + noodle]; }, [&](size_t index) -> double & { return output(index*static_cast<unsigned>(pow(8, D-1)) + noodle); });
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
    double tmp[static_cast<unsigned>(pow(8, D))] {};
    for (size_t slice = 0; slice < 8; slice++) {
      idct<D-1>([&](size_t index) -> double { return input(slice*static_cast<unsigned>(pow(8, D-1)) + index); }, [&](size_t index) -> double & { return tmp[slice*static_cast<unsigned>(pow(8, D-1)) + index]; });
    }
    for (size_t noodle = 0; noodle < static_cast<unsigned>(pow(8, D-1)); noodle++) {
      idct<1>([&](size_t index) -> double { return tmp[index*static_cast<unsigned>(pow(8, D-1)) + noodle]; }, [&](size_t index) -> double & { return output(index*static_cast<unsigned>(pow(8, D-1)) + noodle); });
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
