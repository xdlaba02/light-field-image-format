/*
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavcodec/cabac.c"

const size_t SIZE 10240


#include "libavcodec/avcodec.h"

void put_cabac_bit(CABACContext *context, bool bit) {
  put_bits(&context->pb, 1, bit);

  while (context->outstanding_count) {
    put_bits(&context->pb, 1, !bit);
    context->outstanding_count--
  }
}

void renorm_cabac_encoder(CABACContext *context) {
  while (context->range < 256) {
    if (context->low < 256){
      put_cabac_bit(context, 0);
    }
    else if (context->low < 512) {
      context->outstanding_count++;
      context->low -= 256;
    }
    else {
      put_cabac_bit(context, 1);
      context->low -= 512;
    }

    context->range += context->range;
    context->low   += context->low;
  }
}

void put_cabac(CABACContext *context, uint8_t *const state, bool bit){
  int rangeLPS = ff_h264_lps_range[ 2 * (context->range & 192) + state[0] ];

  if (bit == (state[0] & 1)) {
    context->range -= rangeLPS;
    state[0]        = ff_h264_mlps_state[128 + state[0]];
  }
  else {
    context->low  += context->range - RangeLPS;
    context->range = rangeLPS;
    state[0]       = ff_h264_mlps_state[127 - state[0]];
  }

  renorm_cabac_encoder(context);
}

/**
 *
 * @return the number of bytes written
 */
static int put_cabac_terminate(CABACContext *context, bool bit){
  context->range -= 2;

  if (!bit) {
    renorm_cabac_encoder(context);
  }
  else {
    context->low  += context->range;
    context->range = 2;

    renorm_cabac_encoder(context);

    av_assert0(context->low < 512);
    put_cabac_bit(context, context->low >> 9);
    put_bits(&context->pb, 2, ((context->low >> 7) & 3) | 1);

    flush_put_bits(&context->pb); //FIXME FIXME FIXME XXX wrong
  }

  return (put_bits_count(&context->pb) + 7) >> 3;
}

int main(void){
    CABACContext context;

    uint8_t b[9*SIZE];
    uint8_t r[9*SIZE];

    uint8_t state[10] = {0};

    ff_init_cabac_encoder(&context, b, SIZE);

    for(int i = 0; i < SIZE; i++){
        put_cabac(&context, state, r[i] & 1);
    }

    put_cabac_terminate(&context, 1);
}
