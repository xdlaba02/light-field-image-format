#include "lfiflib.h"
#include "lfif_encoder.h"
#include "lfif_decoder.h"

using namespace std;

template int LFIFCompress<2>(const uint8_t *, const uint64_t *, uint8_t, const char *);
template int LFIFCompress<3>(const uint8_t *, const uint64_t *, uint8_t, const char *);
template int LFIFCompress<4>(const uint8_t *, const uint64_t *, uint8_t, const char *);

template int LFIFDecompress<2>(const char *, vector<uint8_t> &, uint64_t *);
template int LFIFDecompress<3>(const char *, vector<uint8_t> &, uint64_t *);
template int LFIFDecompress<4>(const char *, vector<uint8_t> &, uint64_t *);
