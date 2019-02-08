/******************************************************************************\
* SOUBOR: shift.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef SHIFT_H
#define SHIFT_H

template<size_t D>
inline YCbCrDataBlock<D> shiftBlock(YCbCrDataBlock<D> input) {
  for (size_t i = 0; i < constpow(8, D); i++) {
    input[i] -= 128;
  }
  return input;
}

template<size_t D>
inline YCbCrDataBlock<D> deshiftBlock(YCbCrDataBlock<D> input) {
  for (auto &pixel: input) {
    pixel += 128;
  }
  return input;
}

#endif
