#ifndef CODEC_H
#define CODEC_H

#include <cstdint>

class MyJPEG;
class Bitmap;

MyJPEG *encode(Bitmap *image);
Bitmap *decode(MyJPEG *data);

void fct(int8_t input[8][8], double output[8][8]);


#endif
