#ifndef CODEC_H
#define CODEC_H

#include "bitmap.h"

#include <string>
#include <cstdint>
#include <vector>

class HuffmanTable;
class JPEG2D;

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

const Block<uint8_t> universal_quantization_table = {
  16,11,10,16,24 ,40 ,51 ,61,
  12,12,14,19,26 ,58 ,60 ,55,
  14,13,16,24,40 ,57 ,69 ,56,
  14,17,22,29,51 ,87 ,80 ,62,
  18,22,37,56,68 ,109,103,77,
  24,35,55,64,81 ,104,113,92,
  49,64,78,87,103,121,120,101,
  72,92,95,98,112,100,103,99
};

void saveJPEG(std::string filename, JPEG2D &jpeg);
void loadJPEG(std::string filename, JPEG2D &jpeg);

void jpegEncode(const BitmapRGB &input, const uint8_t quality, JPEG2D &output);
void RGBToYCbCr(const BitmapRGB &input, BitmapG &Y, BitmapG &Cb, BitmapG &Cr);
void scaleQuantizationTable(const Block<uint8_t> &input, const uint8_t quality, Block<uint8_t> &output);
void fillHuffmanTables(const std::vector<RLTuple> &input, HuffmanTable &DC, HuffmanTable &AC);

void encodeChannel(const BitmapG &input, const Block<uint8_t> &quant_table, std::vector<RLTuple> &output);
void splitToBlocks(const BitmapG &input, BitmapG &output);

void fct(const Block<uint8_t> &input, Block<double> &output);
void quantize(const Block<double> &input, const Block<uint8_t> &quant_table, Block<int8_t> &output);
void zigzag(const Block<int8_t> &input, Block<int8_t> &output);
void runlengthEncode(const Block<int8_t> &input, std::vector<RLTuple> &output);

uint8_t RGBtoY(uint8_t R, uint8_t G, uint8_t B);
uint8_t RGBtoCb(uint8_t R, uint8_t G, uint8_t B);
uint8_t RGBtoCr(uint8_t R, uint8_t G, uint8_t B);

#endif
