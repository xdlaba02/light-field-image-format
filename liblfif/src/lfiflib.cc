#include "lfiflib.h"
#include "lfif_encoder.h"

#include <cmath>

using namespace std;

int LFIFCompress(const LFIFCompressStruct *lfif, const char *output_file_name) {
  uint64_t img_dims[5] {};

  switch (lfif->method) {
    case LFIF_2D:
      img_dims[0] = lfif->image_width;
      img_dims[1] = lfif->image_height;
      img_dims[2] = lfif->image_count;
      return LFIFCompress<2>(lfif->rgb_data_24, img_dims, lfif->quality, output_file_name);
    break;

    case LFIF_3D:
      img_dims[0] = lfif->image_width;
      img_dims[1] = lfif->image_height;
      img_dims[2] = lfif->image_count;
      img_dims[3] = 1;
      return LFIFCompress<3>(lfif->rgb_data_24, img_dims, lfif->quality, output_file_name);
    break;

    case LFIF_4D:
      img_dims[0] = lfif->image_width;
      img_dims[1] = lfif->image_height;
      img_dims[2] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[3] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[4] = 1;
      return LFIFCompress<4>(lfif->rgb_data_24, img_dims, lfif->quality, output_file_name);
    break;

  }

  return -1;
}
