/******************************************************************************\
* SOUBOR: lfif.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef LFIF_H
#define LFIF_H

#include "dct.h"

#include <cstdint>
#include <array>
#include <vector>
#include <map>

using namespace std;

using RGBDataUnit = uint8_t;
using YCbCrDataUnit = float;
using QuantizedDataunit = int16_t;
using RefereceBlockUnit = double;
using QuantTableUnit = uint8_t;
using TraversalTableUnit = uint16_t;

using RunLengthZeroesCountUnit = uint8_t;
using RunLengthAmplitudeUnit = int16_t;

using HuffmanClass = uint8_t;
using HuffmanSymbol = uint8_t;

struct RGBDataPixel {
  RGBDataUnit r;
  RGBDataUnit g;
  RGBDataUnit b;
};

struct RunLengthPair {
  RunLengthZeroesCountUnit zeroes;
  RunLengthAmplitudeUnit amplitude;
};

struct HuffmanTable {
  vector<uint8_t> counts;
  vector<uint8_t> symbols;
};

using RGBData = vector<uint8_t>;

using RunLengthEncodedBlock = vector<RunLengthPair>;

using HuffmanCodeword = vector<bool>;
using HuffmanWeights = map<HuffmanSymbol, uint64_t>;
using HuffmanCodelengths = vector<pair<uint64_t, uint8_t>>;
using HuffmanMap = map<uint8_t, HuffmanCodeword>;

template<typename T, uint8_t D>
using Block = array<T, static_cast<size_t>(constpow(8, D))>;

template<size_t D>
using RGBDataBlock = Block<RGBDataPixel, D>;

template<size_t D>
using YCbCrDataBlock = Block<YCbCrDataUnit, D>;

template<size_t D>
using TransformedBlock = Block<DCTDataUnit, D>;

template<size_t D>
using QuantizedBlock = Block<QuantizedDataunit, D>;

template<size_t D>
using RefereceBlock = Block<RefereceBlockUnit, D>;

template<size_t D>
using TraversedBlock = QuantizedBlock<D>;

template<size_t D>
using QuantTable = Block<QuantTableUnit, D>;

template<size_t D>
using TraversalTable = Block<TraversalTableUnit, D>;

#endif
