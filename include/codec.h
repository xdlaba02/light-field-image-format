#ifndef CODEC_H
#define CODEC_H

#include "bitmap.h"

#include <cstdint>
#include <vector>

class ImageJPEG;

using Block = uint64_t[8][8];

bool jpegEncode(const BitmapRGB &rgb, const uint8_t quality, ImageJPEG &jpeg);
bool jpegDecode(const ImageJPEG &jpeg, BitmapRGB &rgb);


void RGBToYCbCr(const BitmapRGB &rgb, BitmapG &Y, BitmapG &Cb, BitmapG &Cr);
void getBlock(uint64_t x, uint64_t y, BitmapG &input, uint8_t output[8][8]);
void fct(uint8_t input[8][8], double output[8][8]);
void quantize(double input[8][8], int8_t output[8][8]);
void zigzag(int8_t input[8][8], int8_t *output);
//void runlength(int8_t input[64], std::vector<RunLengthData> &output);

#endif
