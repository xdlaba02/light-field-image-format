#ifndef CODEC_H
#define CODEC_H

#include "bitmap.h"

#include <cstdint>
#include <vector>

class ImageJPEG;

template<typename T>
using Block = T[64];

struct RLTuple {
  RLTuple(uint8_t z, int16_t a):zeroes(z), amplitude(a) {}
  uint8_t zeroes;
  int16_t amplitude;
};

enum ACDCState {
  STATE_DC,
  STATE_AC
};

void jpegEncode(const BitmapRGB &input, const uint8_t quality, ImageJPEG &output);
void RGBToYCbCr(const BitmapRGB &input, BitmapG &Y, BitmapG &Cb, BitmapG &Cr);

void encodeChannel(const BitmapG &input, const uint8_t quality, std::vector<RLTuple> &output);
void splitToBlocks(const BitmapG &input, BitmapG &output);

void encodeBlock(const Block<uint8_t> &input, const uint8_t quality, std::vector<RLTuple> &output);

void fct(const Block<uint8_t> &input, Block<double> &output);
void quantize(const Block<double> &input, const uint8_t quality, Block<int8_t> &output);
void zigzag(const Block<int8_t> &input, Block<int8_t> &output);
void runlengthEncode(const Block<int8_t> &input, std::vector<RLTuple> &output);

uint8_t RGBtoY(uint8_t R, uint8_t G, uint8_t B);
uint8_t RGBtoCb(uint8_t R, uint8_t G, uint8_t B);
uint8_t RGBtoCr(uint8_t R, uint8_t G, uint8_t B);

#endif
