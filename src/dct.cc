#include "dct.h"

#include <cmath>

void dct1(function<double(uint8_t index)> input, function<double &(uint8_t index)> output) {
  for (uint8_t x = 0; x < 8; x++) {
    output(0) += input(x);
  }
  output(0) *= 1/sqrt(2);

  for (uint8_t u = 1; u < 8; u++) {
    for (uint8_t x = 0; x < 8; x++) {
      output(u) +=  input(x) * cos(((2 * x + 1) * u * M_PI ) / 16);
    }
  }
}

void dct2(const array<int8_t, 8*8> &input, array<double, 8*8> &output) {
  double tmp[8*8] {};
  for (uint8_t x = 0; x < 8; x++) {
    dct1([&](uint8_t index) -> double { return input[index*8+x]; }, [&](uint8_t index) -> double & {   return tmp[index*8+x]; });
  }
  for (uint8_t y = 0; y < 8; y++) {
    dct1([&](uint8_t index) -> double { return   tmp[y*8+index]; }, [&](uint8_t index) -> double & { return output[y*8+index]; });
  }
}

void dct3(const array<int8_t, 8*8*8> &input, array<double, 8*8*8> &output) {
  double tmp[8*8] {};
  for (uint8_t y = 0; y < 8; y++) {
    for (uint8_t x = 0; x < 8; x++) {
      dct1([&](uint8_t index) -> double { return input[index*8*8 + y*8 + x]; }, [&](uint8_t index) -> double & { return output[index*8*8 + y*8 + x]; });
    }
  }
  for (uint8_t x = 0; x < 8; x++) {
    for (uint8_t z = 0; z < 8; z++) {
      dct1([&](uint8_t index) -> double { return output[z*8*8 + index*8 + x]; }, [&](uint8_t index) -> double & { return tmp[z*8*8 + index*8 + x]; });
    }
  }
  for (uint8_t z = 0; z < 8; z++) {
    for (uint8_t y = 0; y < 8; y++) {
      dct1([&](uint8_t index) -> double { return tmp[z*8*8 + y*8 + index]; }, [&](uint8_t index) -> double & { return output[z*8*8 + y*8 + index]; });
    }
  }
}

void dct4(const array<int8_t, 8*8*8*8> &input, array<double, 8*8*8*8> &output) {
  double tmp[8*8] {};
  for (uint8_t xi = 0; xi < 8; xi++) {
    for (uint8_t y = 0; y < 8; y++) {
      for (uint8_t x = 0; x < 8; x++) {
        dct1([&](uint8_t index) -> double { return input[index*8*8*8 + xi*8*8 + y*8 + x]; }, [&](uint8_t index) -> double & { return tmp[index*8*8*8 + xi*8*8 + y*8 + x]; });
      }
    }
  }
  for (uint8_t y = 0; y < 8; y++) {
    for (uint8_t x = 0; x < 8; x++) {
      for (uint8_t yi = 0; yi < 8; yi++) {
        dct1([&](uint8_t index) -> double { return tmp[yi*8*8*8 + index*8*8 + y*8 + x]; }, [&](uint8_t index) -> double & { return output[yi*8*8*8 + index*8*8 + y*8 + x]; });
      }
    }
  }
  for (uint8_t x = 0; x < 8; x++) {
    for (uint8_t yi = 0; yi < 8; yi++) {
      for (uint8_t xi = 0; xi < 8; xi++) {
        dct1([&](uint8_t index) -> double { return output[yi*8*8*8 + xi*8*8 + index*8 + x]; }, [&](uint8_t index) -> double & { return tmp[yi*8*8*8 + xi*8*8 + index*8 + x]; });
      }
    }
  }
  for (uint8_t yi = 0; yi < 8; yi++) {
    for (uint8_t xi = 0; xi < 8; xi++) {
      for (uint8_t y = 0; y < 8; y++) {
        dct1([&](uint8_t index) -> double { return tmp[yi*8*8*8 + xi*8*8 + y*8 + index]; }, [&](uint8_t index) -> double & { return output[yi*8*8*8 + xi*8*8 + y*8 + index]; });
      }
    }
  }
}


void idct1(function<double(uint8_t index)> input, function<double &(uint8_t index)> output) {
  for (uint8_t x = 0; x < 8; x++) {
    output(x) = input(0) * 1/sqrt(2);

    for (uint8_t u = 1; u < 8; u++) {
      output(x) += input(u) * cos(((2 * x + 1) * u * M_PI) / 16);
    }
  }
}

void idct2(const array<double, 8*8> &input, array<int8_t, 8*8> &output) {
  double tmp[8*8] {};
  double tmp2[8*8] {};
  for (uint8_t x = 0; x < 8; x++) {
    idct1([&](uint8_t index) -> double { return input[index*8+x]; }, [&](uint8_t index) -> double & { return tmp[index*8+x]; });
  }
  for (uint8_t y = 0; y < 8; y++) {
    idct1([&](uint8_t index) -> double { return   tmp[y*8+index]; }, [&](uint8_t index) -> double & { return tmp2[y*8+index]; });
  }

  for (uint8_t i = 0; i < 64; i++) {
    output[i] = tmp2[i];
  }
}
