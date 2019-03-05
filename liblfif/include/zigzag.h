/******************************************************************************\
* SOUBOR: zigzag.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef ZIGZAG_H
#define ZIGZAG_H

#include <cstdint>
#include <vector>

std::vector<size_t> generateZigzagTable2D(int64_t size);
std::vector<size_t> generateZigzagTable3D(int64_t size);
std::vector<size_t> generateZigzagTable4D(int64_t size);

#endif
