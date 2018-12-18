/*******************************************************\
* SOUBOR: lfif.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 21. 10. 2018
\*******************************************************/

#ifndef LFIF_H
#define LFIF_H

#include "constmath.h"
#include "endian.h"
#include "dct.h"

#include <cstdint>
#include <array>
#include <cmath>
#include <vector>
#include <tuple>
#include <algorithm>
#include <map>

using namespace std;

template<typename T, uint8_t D>
using Block = array<T, static_cast<uint16_t>(constpow(8, D))>;

using QualityUnit = uint8_t;
using RGBDataUnit = uint8_t;
using YCbCrDataUnit = float;
using QuantizedDataunit = int16_t;
using RefereceBlockUnit = double;
using QuantTableUnit = uint8_t;
using TraversalTableUnit = uint16_t;

using RGBData = vector<RGBDataUnit>;
using YCbCrData = vector<YCbCrDataUnit>;

template<uint8_t D>
using YCbCrDataBlock = Block<YCbCrDataUnit, D>;

template<uint8_t D>
using TransformedBlock = Block<DCTDataUnit, D>;

template<uint8_t D>
using QuantizedBlock = Block<QuantizedDataunit, D>;

template<uint8_t D>
using RefereceBlock = Block<RefereceBlockUnit, D>;

template<uint8_t D>
using TraversedBlock = QuantizedBlock<D>;


template<uint8_t D>
using QuantTable = Block<QuantTableUnit, D>;

template<uint8_t D>
using TraversalTable = Block<TraversalTableUnit, D>;


template<uint8_t D>
using Dimensions = array<size_t, D>;


using RunLengthZeroesCountUnit = uint8_t;
using RunLengthAmplitudeUnit = int16_t;

using HuffmanClass = uint8_t;
using HuffmanSymbol = uint8_t;

struct RunLengthPair {
  RunLengthZeroesCountUnit zeroes;
  RunLengthAmplitudeUnit amplitude;
};

using RunLengthEncodedBlock = vector<RunLengthPair>;

using RunLengthEncodedImage = vector<RunLengthEncodedBlock>;

using HuffmanCodeword = vector<bool>;
using HuffmanWeights = map<HuffmanSymbol, uint64_t>;
using HuffmanCodelengths = vector<pair<uint64_t, uint8_t>>;
using HuffmanMap = map<uint8_t, HuffmanCodeword>;

#endif
