#ifndef DCT_H
#define DCT_H

#include <cstdint>
#include <cmath>

template <typename IF, typename OF>
void fdct1(IF &&input, OF &&output) {
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

template <typename IF, typename OF>
void fdct2(IF &&input, OF &&output) {
  float tmp[8*8] {};
  for (uint8_t y = 0; y < 8; y++) {
    fdct1([&](uint8_t x) -> float { return input(x, y); }, [&](uint8_t x) -> float & { return tmp[y*8+x]; });
  }
  for (uint8_t x = 0; x < 8; x++) {
    fdct1([&](uint8_t y) -> float { return tmp[y*8+x]; }, [&](uint8_t y) -> float & { return output(x, y); });
  }
}

template <typename IF, typename OF>
void fdct3(IF &&input, OF &&output) {
  float tmp[8*8*8] {};
  for (uint8_t z = 0; z < 8; z++) {
    fdct2([&](uint8_t x, uint8_t y) -> float { return input(x, y, z); }, [&](uint8_t x, uint8_t y) -> float & { return tmp[z*8*8 + y*8 + x]; });
  }
  for (uint8_t y = 0; y < 8; y++) {
    for (uint8_t x = 0; x < 8; x++) {
      fdct1([&](uint8_t z) -> float { return tmp[z*8*8 + y*8 + x]; }, [&](uint8_t z) -> float & { return output(x, y, z); });
    }
  }
}

template <typename IF, typename OF>
void fdct4(IF &&input, OF &&output) {
  float tmp[8*8*8*8] {};
  for (uint8_t yi = 0; yi < 8; yi++) {
    fdct3([&](uint8_t x, uint8_t y, uint8_t xi) -> float { return input(x, y, xi, yi); }, [&](uint8_t x, uint8_t y, uint8_t xi) -> float & { return tmp[yi*8*8*8 + xi*8*8 + y*8 + x]; });
  }
  for (uint8_t xi = 0; xi < 8; xi++) {
    for (uint8_t y = 0; y < 8; y++) {
      for (uint8_t x = 0; x < 8; x++) {
        fdct1([&](uint8_t yi) -> float { return tmp[yi*8*8*8 + xi*8*8 + y*8 + x]; }, [&](uint8_t yi) -> float & { return output(x, y, xi, yi); });
      }
    }
  }
}

template <typename IF, typename OF>
void idct1(IF &&input, OF &&output) {
  for (uint8_t x = 0; x < 8; x++) {
    output(x) = input(0) * 1/sqrt(2);

    for (uint8_t u = 1; u < 8; u++) {
      output(x) += input(u) * cos(((2 * x + 1) * u * M_PI) / 16);
    }
  }
}

template <typename IF, typename OF>
void idct2(IF &&input, OF &&output) {
  float tmp[8*8] {};
  for (uint8_t y = 0; y < 8; y++) {
    idct1([&](uint8_t x) -> float { return input(x, y); }, [&](uint8_t x) -> float & { return tmp[y*8+x]; });
  }
  for (uint8_t x = 0; x < 8; x++) {
    idct1([&](uint8_t y) -> float { return tmp[y*8+x]; }, [&](uint8_t y) -> float & { return output(x, y); });
  }
}

template <typename IF, typename OF>
void idct3(IF &&input, OF &&output) {
  float tmp[8*8*8] {};
  for (uint8_t z = 0; z < 8; z++) {
    idct2([&](uint8_t x, uint8_t y) -> float { return input(x, y, z); }, [&](uint8_t x, uint8_t y) -> float & { return tmp[z*8*8 + y*8 + x]; });
  }
  for (uint8_t y = 0; y < 8; y++) {
    for (uint8_t x = 0; x < 8; x++) {
      idct1([&](uint8_t z) -> float { return tmp[z*8*8 + y*8 + x]; }, [&](uint8_t z) -> float & { return output(x, y, z); });
    }
  }
}

template <typename IF, typename OF>
void idct4(IF &&input, OF &&output) {
  float tmp[8*8*8*8] {};
  for (uint8_t yi = 0; yi < 8; yi++) {
    idct3([&](uint8_t x, uint8_t y, uint8_t xi) -> float { return input(x, y, xi, yi); }, [&](uint8_t x, uint8_t y, uint8_t xi) -> float & { return tmp[yi*8*8*8 + xi*8*8 + y*8 + x]; });
  }
  for (uint8_t xi = 0; xi < 8; xi++) {
    for (uint8_t y = 0; y < 8; y++) {
      for (uint8_t x = 0; x < 8; x++) {
        idct1([&](uint8_t yi) -> float { return tmp[yi*8*8*8 + xi*8*8 + y*8 + x]; }, [&](uint8_t yi) -> float & { return output(x, y, xi, yi); });
      }
    }
  }
}

#endif
