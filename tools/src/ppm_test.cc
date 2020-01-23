/******************************************************************************\
* SOUBOR: dynamix_test.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"
#include <meta.h>

#include <iomanip>
#include <iostream>

using namespace std;

int main(int argc, const char *argv[]) {
  if (argc != 3) {
    return 0;
  }

  PPM ppm {};

  if (ppm.mmapPPM(argv[1]) < 0) {
    return 0;
  }

  std::vector<uint8_t> rgb_data {};

  iterate_dimensions<2>(std::array<size_t, 2> {ppm.width(), ppm.height()}, [&](const auto &pos) {
    rgb_data.push_back(ppm[(pos[1] * ppm.width() + pos[0]) * 3 + 0]);
    rgb_data.push_back(ppm[(pos[1] * ppm.width() + pos[0]) * 3 + 1]);
    rgb_data.push_back(ppm[(pos[1] * ppm.width() + pos[0]) * 3 + 2]);
  });

  if (!savePPMs(argv[2], rgb_data.data(), ppm.width(), ppm.height(), ppm.color_depth(), 1)) {
    return 3;
  }
}
