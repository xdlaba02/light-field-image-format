/******************************************************************************\
* SOUBOR: quant_table.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef QUANT_TABLE_H
#define QUANT_TABLE_H

#include "lfiftypes.h"
#include "dct.h"
#include "endian_t.h"
#include "block.h"

#include <istream>
#include <ostream>

using QTABLEUNIT = uint64_t;

template <size_t BS, size_t D>
using QuantTable = Block<QTABLEUNIT, BS, D>;

static constexpr QuantTable<8, 2> base_luma = {
  16,  11,  10,  16,  24,  40,  51,  61,
  12,  12,  14,  19,  26,  58,  60,  55,
  14,  13,  16,  24,  40,  57,  69,  56,
  14,  17,  22,  29,  51,  87,  80,  62,
  18,  22,  37,  56,  68, 109, 103,  77,
  24,  35,  55,  64,  81, 104, 113,  92,
  49,  64,  78,  87, 103, 121, 120, 101,
  72,  92,  95,  98, 112, 100, 103,  99
};

static constexpr QuantTable<8, 2> base_chroma = {
  17, 18, 24, 47, 99, 99, 99, 99,
  18, 21, 26, 66, 99, 99, 99, 99,
  24, 26, 56, 99, 99, 99, 99, 99,
  47, 66, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99
};

template <size_t BSIN, size_t D, size_t BSOUT>
constexpr QuantTable<BSOUT, D> scaleFillNear(const QuantTable<BSIN, D> &input) {
  QuantTable<BSOUT, D> output {};

  auto inputF = [&](size_t index) {
    return input[index];
  };

  auto outputF = [&](size_t index, const auto &value) {
    output[index] = value;
  };

  size_t dims[D] {};
  std::fill(dims, dims + D, BSIN);
  getBlock<BSOUT, D>(inputF, 0, dims, outputF);

  return output;
}

template <size_t BSIN, size_t D, size_t BSOUT>
constexpr QuantTable<BSOUT, D> scaleByDCT(const QuantTable<BSIN, D> &input) {
  Block<DCTDATAUNIT, BSIN, D>  input_coefs  {};
  Block<DCTDATAUNIT, BSOUT, D> output_coefs {};
  QuantTable<BSOUT, D>         output       {};

  auto finputF = [&](size_t index) -> DCTDATAUNIT {
    return input[index];
  };

  auto foutputF = [&](size_t index) -> DCTDATAUNIT & {
    return input_coefs[index];
  };

  fdct<BSIN, D>(finputF, foutputF);

  auto iinputF = [&](size_t index) -> DCTDATAUNIT {
    size_t real_index = 0;

    for (size_t j = D; j > 0; j--) {
      size_t ii = (index % constpow(BSOUT, j)) / constpow(BSOUT, j - 1);
      if (ii >= BSIN) {
        return 0;
      }
      else {
        real_index *= BSIN;
        real_index += ii;
      }
    }

    return input_coefs[real_index];
  };

  auto ioutputF = [&](size_t index) -> DCTDATAUNIT & {
    return output_coefs[index];
  };

  idct<BSOUT, D>(iinputF, ioutputF);

  for (size_t i = 0; i < constpow(BSOUT, D); i++) {
    output[i] = std::round(output_coefs[i]);
  }

  return output;
}

template <size_t BS, size_t D>
QuantTable<BS, D> applyQualityCoefficient(QuantTable<BS, D> table, float scale_coef) {
  for (size_t i = 0; i < constpow(BS, D); i++) {
    table[i] = std::round(table[i] * scale_coef);
  }
  return table;
}

template <size_t BS, size_t D>
QuantTable<BS, D> applyQuality(const QuantTable<BS, D> &input, float quality) {
  float scale_coef = quality < 50 ? (50.0 / quality) : (200.0 - 2 * quality) / 100;
  return applyQualityCoefficient<BS, D>(input, scale_coef);
}

template <size_t BS, size_t D>
QuantTable<BS, D> uniformTable(QTABLEUNIT value) {
  QuantTable<BS, D> output {};
  output.fill(value);
  return output;
}

template <size_t BS, size_t DIN, size_t DOUT>
QuantTable<BS, DOUT> copyTable(const QuantTable<BS, DIN> &input) {
  QuantTable<BS, DOUT> output {};

  static_assert(DIN <= DOUT);

  for (size_t y = 0; y < constpow(BS, DOUT - DIN); y++) {
    for (size_t x = 0; x < constpow(BS, DIN); x++) {
      output[y * constpow(BS, DIN) + x] = input[x];
    }
  }

  return output;
}

template <size_t BS, size_t DIN, size_t DOUT>
QuantTable<BS, DOUT> averageDiagonalTable(const QuantTable<BS, DIN> &input) {
  double diagonals_sum[DIN * (BS - 1) + 1]   {};
  size_t diagonals_cnt[DIN * (BS - 1) + 1]   {};
  QuantTable<BS, DOUT> output {};

  for (size_t i = 0; i < constpow(BS, DIN); i++) {
    size_t diagonal = 0;
    for (size_t j = 0; j < DIN; j++) {
      diagonal += (i % constpow(BS, j + 1)) / constpow(BS, j);
    }

    diagonals_sum[diagonal] += input[i];
    diagonals_cnt[diagonal]++;
  }

  for (size_t i = 0; i < constpow(BS, DOUT); i++) {
    size_t diagonal = 0;
    for (size_t j = 0; j < DOUT; j++) {
      diagonal += (i % constpow(BS, j + 1)) / constpow(BS, j);
    }

    if (diagonal >= DIN * (BS - 1) + 1) {
      diagonal = DIN * (BS - 1) + 1 - 1;
    }

    output[i] = std::round(diagonals_sum[diagonal] / diagonals_cnt[diagonal]);
  }

  return output;
}

template <size_t BS, size_t D>
QuantTable<BS, D> clampTable(QuantTable<BS, D> table, float min, float max) {
  for (size_t i = 0; i < constpow(BS, D); i++) {
    table[i] = std::clamp<float>(table[i], min, max);
  }
  return table;
}

template <size_t BS, size_t D>
void writeToStream(const QuantTable<BS, D> &table, std::ostream &stream) {
  for (size_t i = 0; i < constpow(BS, D); i++) {
    writeValueToStream<uint8_t>(table[i], stream);
  }
}

template <size_t BS, size_t D>
QuantTable<BS, D> readFromStream(std::istream &stream) {
  QuantTable<BS, D> output {};
  for (size_t i = 0; i < constpow(BS, D); i++) {
    output[i] = readValueFromStream<uint8_t>(stream);
  }
  return output;
}


#endif
