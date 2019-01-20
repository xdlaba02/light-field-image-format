/******************************************************************************\
* SOUBOR: zigzag.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef ZIGZAG_H
#define ZIGZAG_H

#include <cstdint>
#include <vector>

#include <iostream>
#include <algorithm>

using namespace std;

vector<size_t> generateZigzagTable(uint64_t size);

template<size_t D>
struct constructZigzagTable {
  template <typename F>
  constructZigzagTable(uint64_t size, F &&output) {

  }
};

template<>
struct constructZigzagTable<4> {
  template <typename F>
  constructZigzagTable(uint64_t size, F &&output) {
    vector<pair<size_t, size_t>> srt(pow(size, 4));

    for (size_t zz = 0; zz < size; zz++) {
      for (size_t z = 0; z < size; z++) {
        for (size_t y = 0; y < size; y++) {
          for (size_t x = 0; x < size; x++) {
            srt[zz * pow(size, 3) + z * pow(size, 2) + y * pow(size, 1) + x].first = x + y + z + zz;

            srt[zz * pow(size, 3) + z * pow(size, 2) + y * pow(size, 1) + x].first *= 4*size + 1;

            if ((x+y+z+zz) % 2) {
              srt[zz * pow(size, 3) + z * pow(size, 2) + y * pow(size, 1) + x].first += y + z + zz;
            }
            else {
              srt[zz * pow(size, 3) + z * pow(size, 2) + y * pow(size, 1) + x].first -= y + z + zz;
            }

          }

          srt[zz * pow(size, 3) + z * pow(size, 2) + y * pow(size, 1) + x].first *= 4*size + 1;

          if ((y+z+zz) % 2) {
            srt[zz * pow(size, 3) + z * pow(size, 2) + y * pow(size, 1) + x].first += z + zz;
          }
          else {
            srt[zz * pow(size, 3) + z * pow(size, 2) + y * pow(size, 1) + x].first -= z + zz;
          }

          srt[zz * pow(size, 3) + z * pow(size, 2) + y * pow(size, 1) + x].first *= 4*size + 1;

          if ((z+zz) % 2) {
            srt[zz * pow(size, 3) + z * pow(size, 2) + y * pow(size, 1) + x].first -= zz;
          }
          else {
            srt[zz * pow(size, 3) + z * pow(size, 2) + y * pow(size, 1) + x].first += zz;
          }
        }
      }
    }

    for (size_t i = 0; i < srt.size(); i++) {
      srt[i].second = i;
    }

    sort(srt.begin(), srt.end());

    for (size_t i = 0; i < srt.size(); i++) {
      output(srt[i].second) = i;
    }
  }
};

template<>
struct constructZigzagTable<3> {
  template <typename F>
  constructZigzagTable(uint64_t size, F &&output) {
    for (size_t z = 0; z < size; z++) {
      for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
          output(z * size * size + y * size + x) = (x + y + z) * (3*size+1);

          if ((x+y+z) % 2) {
            output(z * size * size + y * size + x) += y + z;
          }
          else {
            output(z * size * size + y * size + x) -= y + z;
          }

          output(z * size * size + y * size + x) *= (3*size+1);

          if ((y+z) % 2) {
            output(z * size * size + y * size + x) -= z;
          }
          else {
            output(z * size * size + y * size + x) += z;
          }

        }
      }
    }

    vector<pair<size_t, size_t>> srt(size * size * size);
    for (size_t i = 0; i < srt.size(); i++) {
      srt[i].first = output(i);
      srt[i].second = i;
    }
    sort(srt.begin(), srt.end());
    for (size_t i = 0; i < srt.size(); i++) {
      output(srt[i].second) = i;
    }
  }
};

template<>
struct constructZigzagTable<D> {
  template <typename F>
  constructZigzagTable(uint64_t size, F &&output) {
    for (size_t y = 0; y < size; y++) {
      auto outF = [&](size_t x) -> size_t & {
        return output(y * pow(size, D-1) + x);
      };
      constructZigzagTable<D-1>(size, outF);
    }

    for (size_t x = 0; x < size; x++) {
      auto outF = [&](size_t x) -> size_t & {
        return output(y * pow(size, D-1) + x);
      };
      constructZigzagTable<1>(size, outF);
    }

    for (size_t y = 0; y < size; y++) {
      for (size_t x = 0; x < size; x++) {
        output(y * size + x) = (x + y) * 10;
        if ((x+y) % 2) {
          output(y * size + x) += y;
        }
        else {
          output(y * size + x) -= y;
        }
      }
    }
  }
};

template<>
struct constructZigzagTable<1> {
  template <typename F>
  constructZigzagTable(uint64_t size, F &&output) {
    for (size_t x = 0; x < size; x++) {
      output(x) += x;
    }
  }
};

#endif
