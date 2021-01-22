/**
* @file quant_table.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Module for generating quantization matrices.
*/

#pragma once

#include "dct.h"
#include "endian_t.h"
#include "block.h"

#include <istream>
#include <ostream>

/**
 * @brief Quantization matrix type.
 */
template <size_t D>
using QuantTable = DynamicBlock<uint64_t, D>;

/**
 * @brief Base luma matrix used in libjpeg implementation. Values corresponds to quality of 50.
 */
static constexpr std::array<uint64_t, 64> base_luma {
  16,  11,  10,  16,  24,  40,  51,  61,
  12,  12,  14,  19,  26,  58,  60,  55,
  14,  13,  16,  24,  40,  57,  69,  56,
  14,  17,  22,  29,  51,  87,  80,  62,
  18,  22,  37,  56,  68, 109, 103,  77,
  24,  35,  55,  64,  81, 104, 113,  92,
  49,  64,  78,  87, 103, 121, 120, 101,
  72,  92,  95,  98, 112, 100, 103,  99
};

/**
 * @brief Base chroma matrix used in libjpeg implementation. Values corresponds to quality of 50.
 */
static constexpr std::array<uint64_t, 64> base_chroma {
  17, 18, 24, 47, 99, 99, 99, 99,
  18, 21, 26, 66, 99, 99, 99, 99,
  24, 26, 56, 99, 99, 99, 99, 99,
  47, 66, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99
};

inline QuantTable<2> baseLuma() {
  QuantTable<2> output({8, 8});
  for (size_t i {}; i < 64; i++) {
    output[i] = base_luma[i];
  }
  return output;
}

inline QuantTable<2> baseChroma() {
  QuantTable<2> output({8, 8});
  for (size_t i {}; i < 64; i++) {
    output[i] = base_chroma[i];
  }
  return output;
}

/**
 * @brief Function which scales matrix values by a coefficient.
 * @param table The matrix to be scaled.
 * @param scale_coef The scaling coefficient.
 * @return Scaled matrix.
 */
template <size_t D>
void applyQualityCoefficient(QuantTable<D> &table, float scale_coef) {
  for (size_t i = 0; i < table.stride(D); i++) {
    table[i] = std::round(table[i] * scale_coef);
  }
}

/**
 * @brief Function which applies quality coefficient to a matrix.
 * @param input The matrix to be scaled.
 * @param quality The desired quality between 1.0 and 100.0.
 * @return Scaled matrix.
 */
template <size_t D>
void applyQuality(QuantTable<D> &input, float quality) {
  float scale_coef = quality < 50 ? (50.0 / quality) : (200.0 - 2 * quality) / 100;
  applyQualityCoefficient<D>(input, scale_coef);
}

/**
 * @brief Function which generates uniform matrix.
 * @param value The uniform value.
 * @return Uniform matrix.
 */
template <size_t D>
void uniformTable(uint64_t value, QuantTable<D> &output) {
  output.fill(value);
}


/**
 * @brief Function which clamp matrix values to specific range.
 * @param table The matrix to be clamped.
 * @param min Minimum clamped value.
 * @param max Maximum clamped value.
 * @return Clamped matrix.
 */
template <size_t D>
void clampTable(QuantTable<D> &table, float min, float max) {
  for (size_t i {}; i < table.stride(D); i++) {
    table[i] = std::clamp<float>(table[i], min, max);
  }
}

/**
 * @brief Function which writes matrix to stream.
 * @param table The matrix to be written.
 * @param stream The stream to which the matrix shall be written.
 */
template <size_t D>
void writeQuantToStream(const QuantTable<D> &table, std::ostream &stream) {
  for (size_t i {}; i < table.stride(D); i++) {
    writeValueToStream<uint8_t>(table[i], stream);
  }
}

/**
 * @brief Function which reads the matrix from stream.
 * @param stream The stream from which the matrix shall be read.
 * @return The read matrix.
 */
template <size_t D>
QuantTable<D> readQuantFromStream(const std::array<size_t, D> &BS, std::istream &stream) {
  QuantTable<D> table(BS);
  for (size_t i {}; i < table.stride(D); i++) {
    table[i] = readValueFromStream<uint8_t>(stream);
  }
  return table;
}
