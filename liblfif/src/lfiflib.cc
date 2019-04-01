#include "lfiflib.h"
#include "lfif_encoder.h"
#include "lfif_decoder.h"
#include "lfiftypes.h"

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

  writeValueToStream<uint16_t>(output, lfif->max_rgb_value);
  writeValueToStream<uint64_t>(output, lfif->image_width);
  writeValueToStream<uint64_t>(output, lfif->image_height);
  writeValueToStream<uint64_t>(output, lfif->image_count);

  img_dims[0] = lfif->image_width;
  img_dims[1] = lfif->image_height;
  switch (lfif->method) {
    case LFIF_2D:
      img_dims[2] = lfif->image_count;

      if (lfif->max_rgb_value < 256) {
        return LFIFCompress<2, uint8_t>(static_cast<const uint8_t *>(rgb_data), img_dims, lfif->quality, lfif->max_rgb_value, output);
      }
      else {
        return LFIFCompress<2, uint16_t>(static_cast<const uint16_t *>(rgb_data), img_dims, lfif->quality, lfif->max_rgb_value, output);
      }
    break;

    case LFIF_3D:
      img_dims[2] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[3] = static_cast<uint64_t>(sqrt(lfif->image_count));

      if (lfif->max_rgb_value < 256) {
        return LFIFCompress<3, uint8_t>(static_cast<const uint8_t *>(rgb_data), img_dims, lfif->quality, lfif->max_rgb_value, output);
      }
      else {
        return LFIFCompress<3, uint16_t>(static_cast<const uint16_t *>(rgb_data), img_dims, lfif->quality, lfif->max_rgb_value, output);
      }
    break;

    case LFIF_4D:
      img_dims[2] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[3] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[4] = 1;

      if (lfif->max_rgb_value < 256) {
        return LFIFCompress<4, uint8_t>(static_cast<const uint8_t *>(rgb_data), img_dims, lfif->quality, lfif->max_rgb_value, output);
      }
      else {
        return LFIFCompress<4, uint16_t>(static_cast<const uint16_t *>(rgb_data), img_dims, lfif->quality, lfif->max_rgb_value, output);
      }
    break;

    default:
      return -1;
    break;

  }

  return -1;
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
