#include "lfiflib.h"
#include "lfif_encoder.h"
#include "lfif_decoder.h"
#include "lfiftypes.h"
#include "colorspace.h"

#include <cmath>

#include <fstream>

using namespace std;

int LFIFCompress(LFIFCompressStruct *lfif, const void *rgb_data) {
  ofstream output         {};
  uint64_t img_dims[5]    {};
  size_t   last_slash_pos {};

  last_slash_pos = string(lfif->output_file_name).find_last_of('/');
  if (last_slash_pos != string::npos) {
    string command("mkdir -p " + string(lfif->output_file_name).substr(0, last_slash_pos));
    system(command.c_str());
  }

  output.open(lfif->output_file_name);
  if (output.fail()) {
    return -1;
  }

  switch (lfif->method) {
    case LFIF_2D:
      output.write("LFIF-2D\n", 8);
    break;

    case LFIF_3D:
      output.write("LFIF-3D\n", 8);
    break;

    case LFIF_4D:
      output.write("LFIF-4D\n", 8);
    break;

    default:
      return -1;
    break;
  }

  auto inputF1 = [&](size_t channel, size_t index) -> uint16_t {
    if (lfif->max_rgb_value < 256) {
      return static_cast<const uint8_t *>(rgb_data)[index * 3 + channel];
    }
    else {
      return static_cast<const uint16_t *>(rgb_data)[index * 3 + channel];
    }
  };

  auto inputF2 = [&](size_t channel, size_t index) -> YCBCRUNIT {
    switch (channel) {
      case 0:
        return RGBToY(inputF1(0, index), inputF1(1, index), inputF1(2, index)) - (lfif->max_rgb_value + 1) / 2;
        break;

      case 1:
        return RGBToCb(inputF1(0, index), inputF1(1, index), inputF1(2, index));
        break;

      case 2:
        return RGBToCr(inputF1(0, index), inputF1(1, index), inputF1(2, index));
        break;
    }

    return 0;
  };

  writeValueToStream<uint16_t>(output, lfif->max_rgb_value);
  writeValueToStream<uint64_t>(output, lfif->image_width);
  writeValueToStream<uint64_t>(output, lfif->image_height);
  writeValueToStream<uint64_t>(output, lfif->image_count);

  img_dims[0] = lfif->image_width;
  img_dims[1] = lfif->image_height;
  switch (lfif->method) {
    case LFIF_2D:
      img_dims[2] = lfif->image_count;
      lfifCompress<2>(inputF2, img_dims, lfif->quality, lfif->max_rgb_value, output);
    break;

    case LFIF_3D:
      img_dims[2] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[3] = static_cast<uint64_t>(sqrt(lfif->image_count));

      lfifCompress<3>(inputF2, img_dims, lfif->quality, lfif->max_rgb_value, output);
    break;

    case LFIF_4D:
      img_dims[2] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[3] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[4] = 1;

      lfifCompress<4>(inputF2, img_dims, lfif->quality, lfif->max_rgb_value, output);
    break;

    default:
      return -1;
    break;

  }

  return 0;
}

int LFIFReadHeader(LFIFDecompressStruct *lfif) {
  ifstream input           {};
  char     magic_number[9] {};

  auto getCompressMethod = [](const string &magic_number) {
    if (magic_number == "LFIF-2D\n") return LFIF_2D;
    if (magic_number == "LFIF-3D\n") return LFIF_3D;
    if (magic_number == "LFIF-4D\n") return LFIF_4D;
    return LFIF_METHODS_CNT;
  };

  input.open(lfif->input_file_name);
  if (input.fail()) {
    return -1;
  }

  input.read(magic_number, 8);
  lfif->method = getCompressMethod(string(magic_number));

  lfif->max_rgb_value = readValueFromStream<uint16_t>(input);
  lfif->image_width   = readValueFromStream<uint64_t>(input);
  lfif->image_height  = readValueFromStream<uint64_t>(input);
  lfif->image_count   = readValueFromStream<uint64_t>(input);
  return 0;
}

int LFIFDecompress(LFIFDecompressStruct *lfif, void *rgb_buffer) {
  ifstream input       {};
  uint64_t img_dims[5] {};

  input.open(lfif->input_file_name);
  if (input.fail()) {
    return -1;
  }

  input.ignore(8 + 2 + 3 * 8);

  img_dims[0] = lfif->image_width;
  img_dims[1] = lfif->image_height;
  switch (lfif->method) {
    case LFIF_2D:
      img_dims[2] = lfif->image_count;

      if (lfif->max_rgb_value < 256) {
        return LFIFDecompress<2, uint8_t>(input, img_dims, lfif->max_rgb_value, static_cast<uint8_t *>(rgb_buffer));
      }
      else {
        return LFIFDecompress<2, uint16_t>(input, img_dims, lfif->max_rgb_value, static_cast<uint16_t *>(rgb_buffer));
      }
    break;

    case LFIF_3D:
      img_dims[2] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[3] = static_cast<uint64_t>(sqrt(lfif->image_count));

      if (lfif->max_rgb_value < 256) {
        return LFIFDecompress<3, uint8_t>(input, img_dims, lfif->max_rgb_value, static_cast<uint8_t *>(rgb_buffer));
      }
      else {
        return LFIFDecompress<3, uint16_t>(input, img_dims, lfif->max_rgb_value, static_cast<uint16_t *>(rgb_buffer));
      }
    break;

    case LFIF_4D:
      img_dims[2] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[3] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[4] = 1;

      if (lfif->max_rgb_value < 256) {
        return LFIFDecompress<4, uint8_t>(input, img_dims, lfif->max_rgb_value, static_cast<uint8_t *>(rgb_buffer));
      }
      else {
        return LFIFDecompress<4, uint16_t>(input, img_dims, lfif->max_rgb_value, static_cast<uint16_t *>(rgb_buffer));
      }
    break;

    default:
      return -1;
    break;
  }

  return -1;
}
