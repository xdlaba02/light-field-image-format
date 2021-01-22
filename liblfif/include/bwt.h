/**
* @file bwt.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 1. 9. 2020
* @brief
*/

#pragma once

#include <cstddef>
#include <cstdint>

#include <vector>

size_t bwt(std::vector<int64_t> &string);
void ibwt(std::vector<int64_t> &string, size_t pidx);
