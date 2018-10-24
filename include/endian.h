/*******************************************************\
* SOUBOR: endian.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 21. 10. 2018
\*******************************************************/

#ifndef ENDIAN_H
#define ENDIAN_H

#include <cstdint>

bool amIBigEndian();
uint64_t swapBytes(uint64_t v);
uint64_t toBigEndian(uint64_t v);
uint64_t fromBigEndian(uint64_t v);

#endif
