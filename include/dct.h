#ifndef DCT_H
#define DCT_H

#include <functional>
#include <array>


using namespace std;

void dct1(function<double(uint8_t index)> &input, function<double &(uint8_t index)> &output);
void dct2(const array<int8_t, 8*8> &input, array<double, 8*8> &output);
void dct3(const array<int8_t, 8*8*8> &input, array<double, 8*8*8> &output);
void dct4(const array<int8_t, 8*8*8*8> &input, array<double, 8*8*8*8> &output);

void idct2(const array<double, 8*8> &input, array<int8_t, 8*8> &output);

#endif
