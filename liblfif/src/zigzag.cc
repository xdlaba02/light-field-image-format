/******************************************************************************\
* SOUBOR: zigzag.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "zigzag.h"

void rotate2D(size_t *input, size_t size) {
  std::vector<size_t> output(size * size);

  for (size_t y = 0; y < size; y++) {
    for (size_t x = 0; x < size; x++) {
      output[x * size + y] = input[y * size + x];
    }
  }

  for (size_t i = 0; i < output.size(); i++) {
    input[i] = output[i];
  }
}

void rotate3D(size_t *input, size_t size) {
  std::vector<size_t> output(size * size * size);

  for (size_t z = 0; z < size; z++) {
    for (size_t y = 0; y < size; y++) {
      for (size_t x = 0; x < size; x++) {
        output[(y * size + x) * size + z] = input[(z * size + y) * size + x];
      }
    }
  }

  for (size_t i = 0; i < output.size(); i++) {
    input[i] = output[i];
  }
}

void rotate4D(size_t *input, size_t size) {
  std::vector<size_t> output(size * size * size * size);

  for (size_t h = 0; h < size; h++) {
    for (size_t w = 0; w < size; w++) {
      for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
          output[((w * size + y) * size + x) * size + h] = input[((h * size + w) * size + y) * size + x];
        }
      }
    }
  }

  for (size_t i = 0; i < output.size(); i++) {
    input[i] = output[i];
  }
}

std::vector<size_t> generateZigzagTable2D(int64_t size) {
  std::vector<size_t> table(size * size);

  size_t index = 0;
  int64_t xx = 0;
  int64_t yy = 0;

  while ((xx + yy) <= ((size - 1) * 2)) {
    int64_t x = xx;
    int64_t y = yy;

    while ((y < size) && (x >= 0)) {
      table[y * size + x] = index++;
      x--;
      y++;
    }

    rotate2D(&table[0], size);

    if (xx < size - 1) {
      xx++;
    }
    else {
      yy++;
    }
  }

  return table;
}

std::vector<size_t> generateZigzagTable3D(int64_t size) {
  std::vector<size_t> table(size * size * size);

  size_t index = 0;

  int64_t xxx = 0;
  int64_t yyy = 0;
  int64_t zzz = 0;

  while (xxx + yyy + zzz <= (size - 1) * 3) {
    int64_t xx = xxx;
    int64_t yy = yyy;
    int64_t zz = zzz;

    while ((zz < size) && (xx + yy >= 0)) {
      int64_t x = xx;
      int64_t y = yy;

      while ((y < size) && (x >= 0)) {
        table[(zz * size + y) * size + x] = index++;
        x--;
        y++;
      }

      for (int64_t i = 0; i < size; i++) {
        rotate2D(&table[i * size * size], size);
      }

      if (yy > 0) {
        yy--;
      }
      else{
        xx--;
      }

      zz++;
    }

    rotate3D(&table[0], size);

    if (xxx < size - 1) {
      xxx++;
    }
    else if (yyy < size - 1) {
      yyy++;
    }
    else {
      zzz++;
    }
  }

  return table;
}

std::vector<size_t> generateZigzagTable4D(int64_t size) {
  std::vector<size_t> table(size * size * size * size);

  size_t index = 0;

  int64_t xxxx = 0;
  int64_t yyyy = 0;
  int64_t wwww = 0;
  int64_t hhhh = 0;

  while (xxxx + yyyy + wwww + hhhh <= (size - 1) * 4) {
    int64_t xxx = xxxx;
    int64_t yyy = yyyy;
    int64_t www = wwww;
    int64_t hhh = hhhh;

    while ((hhh < size) && (xxx + yyy + www >= 0)) {
      int64_t xx = xxx;
      int64_t yy = yyy;
      int64_t ww = www;

      while ((ww < size) && (xx + yy >= 0)) {
        int64_t x = xx;
        int64_t y = yy;

        while ((y < size) && (x >= 0)) {
          table[((hhh * size + ww) * size + y) * size + x] = index++;
          x--;

          y++;
        }

        for (int64_t i = 0; i < size * size; i++) {
          rotate2D(&table[i * size * size], size);
        }

        if (yy > 0) {
          yy--;
        }
        else {
          xx--;
        }

        ww++;
      }

      for (int64_t i = 0; i < size; i++) {
        rotate3D(&table[i * size * size * size], size);
      }

      if (www > 0) {
        www--;
      }
      else if (yyy > 0) {
        yyy--;
      }
      else {
        xxx--;
      }

      hhh++;
    }

    rotate4D(&table[0], size);

    if (xxxx < size - 1) {
      xxxx++;
    }
    else if (yyyy < size - 1) {
      yyyy++;
    }
    else if (wwww < size - 1) {
      wwww++;
    }
    else {
      hhhh++;
    }
  }

  return table;
}
