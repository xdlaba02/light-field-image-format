/**
* @file cabac.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 11. 7. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Conxtext-Adaptive Binary Arithmetic Coding.
*/

#ifndef CABAC_H
#define CABAC_H

#include "bitstream.h"

class CABAC {
public:
  CABAC():
  m_low         { 0 },
  m_range       { static_cast<uint16_t>(HALF - 2) }
  {}

  struct contextModel {
    uint8_t m_state : 6;
    uint8_t m_mps   : 1;
  };

protected:
  uint16_t m_low;
  uint16_t m_range;

  const uint8_t  BITS    { 10 };
  const uint16_t ONE     { static_cast<uint16_t>(1 << BITS) };
  const uint16_t HALF    { static_cast<uint16_t>(1 << (BITS - 1)) };
  const uint16_t QUARTER { static_cast<uint16_t>(1 << (BITS - 2)) };

  const uint8_t rangeTabLPS[64][4] {
    {128, 176, 208, 240},
    {128, 167, 197, 227},
    {128, 158, 187, 216},
    {123, 150, 178, 205},
    {116, 142, 169, 195},
    {111, 135, 160, 185},
    {105, 128, 152, 175},
    {100, 122, 144, 166},
    { 95, 116, 137, 158},
    { 90, 110, 130, 150},
    { 85, 104, 123, 142},
    { 81,  99, 117, 135},
    { 77,  94, 111, 128},
    { 73,  89, 105, 122},
    { 69,  85, 100, 116},
    { 66,  80,  95, 110},
    { 62,  76,  90, 104},
    { 59,  72,  86,  99},
    { 56,  69,  81,  94},
    { 53,  65,  77,  89},
    { 51,  62,  73,  85},
    { 48,  59,  69,  80},
    { 46,  56,  66,  76},
    { 43,  53,  63,  72},
    { 41,  50,  59,  69},
    { 39,  48,  56,  65},
    { 37,  45,  54,  62},
    { 35,  43,  51,  59},
    { 33,  41,  48,  56},
    { 32,  39,  46,  53},
    { 30,  37,  43,  50},
    { 29,  35,  41,  48},
    { 27,  33,  39,  45},
    { 26,  31,  37,  43},
    { 24,  30,  35,  41},
    { 23,  28,  33,  39},
    { 22,  27,  32,  37},
    { 21,  26,  30,  35},
    { 20,  24,  29,  33},
    { 19,  23,  27,  31},
    { 18,  22,  26,  30},
    { 17,  21,  25,  28},
    { 16,  20,  23,  27},
    { 15,  19,  22,  25},
    { 14,  18,  21,  24},
    { 14,  17,  20,  23},
    { 13,  16,  19,  22},
    { 12,  15,  18,  21},
    { 12,  14,  17,  20},
    { 11,  14,  16,  19},
    { 11,  13,  15,  18},
    { 10,  12,  15,  17},
    { 10,  12,  14,  16},
    {  9,  11,  13,  15},
    {  9,  11,  12,  14},
    {  8,  10,  12,  14},
    {  8,   9,  11,  13},
    {  7,   9,  11,  12},
    {  7,   9,  10,  12},
    {  7,   8,  10,  11},
    {  6,   8,   9,  11},
    {  6,   7,   9,  10},
    {  6,   7,   8,   9},
    {  2,   2,   2,   2}
  };
  const uint8_t transIdxMPS[64] {
     1,  2,  3,  4,  5,  6,  7,  8,
     9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56,
    57, 58, 59, 60, 61, 62, 62, 63
  };
  const uint8_t transIdxLPS[64] {
     0,  0,  1,  2,  2,  4,  4,  5,
     6,  7,  8,  9,  9, 11, 11, 12,
    13, 13, 15, 15, 16, 16, 18, 18,
    19, 19, 21, 21, 22, 22, 23, 24,
    24, 25, 26, 26, 27, 27, 28, 29,
    29, 30, 30, 30, 31, 32, 32, 33,
    33, 33, 34, 34, 35, 35, 35, 36,
    36, 36, 37, 37, 37, 38, 38, 63
  };
};

class CABACEncoder: public CABAC {
public:
  CABACEncoder():
  m_outstanding { 0 }
  {}

  void encodeBitToStream(contextModel &context, bool bit, OBitstream &stream);
  void encodeBitToStreamBypass(bool bit, OBitstream &stream);
  void terminate();

private:
  size_t m_outstanding;

  void renormalize(OBitstream &stream);
  void putCabacBit(bool bit, OBitstream &stream);
};

class CABACDecoder: public CABAC {
public:
  CABACDecoder():
  m_value { 0 }
  {}

  bool decodeBitFromStream(contextModel &context, IBitstream &stream);
  bool decodeBitFromStreamBypass(IBitstream &stream);

private:
  size_t m_value;

  void renormalize(IBitstream &stream);
};

void CABACEncoder::encodeBitToStream(contextModel &context, bool bit, OBitstream &stream) {
  uint8_t rLPS {};

  rLPS = rangeTabLPS[context.m_state][(m_range >> 6) & 3];
  m_range -= rLPS;

  if (bit != context.m_mps) {
    m_low  += m_range;
    m_range = rLPS;

    if (context.m_state == 0) {
      context.m_mps = !context.m_mps;
    }

    context.m_state = transIdxLPS[context.m_state];
  }
  else {
    context.m_state = transIdxMPS[context.m_state];
  }

  renormalize(stream);
}

void CABACEncoder::encodeBitToStreamBypass(bool bit, OBitstream &stream) {
  m_low <<= 1;
  if (!bit) {
    m_low += m_range;
  }
  if (m_low >= ONE) {
    putCabacBit(1, stream);
    m_low -= ONE;
  }
  else if (m_low < HALF) {
    putCabacBit(0, stream);
  }
  else {
    m_outstanding++;
    m_low -= HALF;
  }
}

void CABACEncoder::putCabacBit(bool bit, OBitstream &stream) {
  stream.writeBit(bit);

  while (m_outstanding) {
    stream.writeBit(!bit);
    m_outstanding--;
  }
}

void CABACEncoder::renormalize(OBitstream &stream) {
  while (m_range < QUARTER) {
    if (m_low >= HALF) {
      putCabacBit(1, stream);
      m_low -= HALF;
    }
    else if (m_low < QUARTER) {
      putCabacBit(0, stream);
    }
    else {
      m_outstanding++;
      m_low -= QUARTER;
    }

    m_low   <<= 1;
    m_range <<= 1;
  }
}

bool CABACDecoder::decodeBitFromStream(contextModel &context, IBitstream &stream) {
  bool    bit  {};
  uint8_t rLPS {};

  rLPS     = rangeTabLPS[context.m_state][(m_range >> 6) & 3];
  m_range -= rLPS;

  if (m_value < m_range) {
    bit             = context.m_mps;
    context.m_state = transIdxMPS[context.m_state];
  }
  else {
    m_value -= m_range;
    m_range = rLPS;
    bit     = !context.m_mps;

    if (context.m_state == 0) {
      context.m_mps = !context.m_mps;
    }

    context.m_state = transIdxLPS[context.m_state];
  }

  renormalize(stream);

  return bit;
}

bool CABACDecoder::decodeBitFromStreamBypass(IBitstream &stream) {
  m_value = (m_value << 1) | stream.readBit();
  if (m_value >= m_range) {
    return 1;
    m_value -= m_range;
  }
  else {
    return 0;
  }
}

void CABACDecoder::renormalize(IBitstream &stream) {
  while (m_range < QUARTER) {
    m_range = m_range << 1;
    m_value = (m_value << 1) | stream.readBit();
  }
}

#endif
