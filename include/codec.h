#ifndef CODEC_H
#define CODEC_H

#include <cstdint>

class MyJPEG;

uint8_t *encodeRGB(uint64_t width, uint64_t height, uint8_t *RGBdata);

void RGBtoYCbCr(uint8_t R,
                uint8_t G,
                uint8_t B,
                uint8_t &Y,
                uint8_t &Cb,
                uint8_t &Cr);

void fct(int8_t input[8][8], double output[8][8]);


#endif
