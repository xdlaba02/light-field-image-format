#include "lfiflib.h"
#include "lfif_encoder.h"
#include "lfif_decoder.h"

#include <cmath>

#include <iostream> //FIXME

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

  switch (lfif->color_space) {
    case RGB24:
      output.write("RGB24  \n", 8);
    break;

    case RGB48:
      output.write("RGB48  \n", 8);
    break;

    default:
      return -1;
    break;
  }

  uint64_t tmp {};

  tmp = htobe64(lfif->image_width);
  output.write(reinterpret_cast<const char *>(&tmp), sizeof(tmp));

  tmp = htobe64(lfif->image_height);
  output.write(reinterpret_cast<const char *>(&tmp), sizeof(tmp));

  tmp = htobe64(lfif->image_count);
  output.write(reinterpret_cast<const char *>(&tmp), sizeof(tmp));


  img_dims[0] = lfif->image_width;
  img_dims[1] = lfif->image_height;
  switch (lfif->method) {
    case LFIF_2D:
      img_dims[2] = lfif->image_count;
      switch (lfif->color_space) {
        case RGB24:
          return LFIFCompress<2, uint8_t, int16_t>(static_cast<const uint8_t *>(rgb_data), img_dims, lfif->quality, output);
        break;

        case RGB48:
          return LFIFCompress<2, uint16_t, int32_t>(static_cast<const uint16_t *>(rgb_data), img_dims, lfif->quality, output);
        break;

        default:
          return -1;
        break;
      }
    break;

    case LFIF_3D:
      img_dims[2] = lfif->image_count;
      img_dims[3] = 1;
      switch (lfif->color_space) {
        case RGB24:
          return LFIFCompress<3, uint8_t, int16_t>(static_cast<const uint8_t *>(rgb_data), img_dims, lfif->quality, output);
        break;

        case RGB48:
          return LFIFCompress<3, uint16_t, int32_t>(static_cast<const uint16_t *>(rgb_data), img_dims, lfif->quality, output);
        break;

        default:
          return -1;
        break;
      }
    break;

    case LFIF_4D:
      img_dims[2] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[3] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[4] = 1;
      switch (lfif->color_space) {
        case RGB24:
          return LFIFCompress<4, uint8_t, int16_t>(static_cast<const uint8_t *>(rgb_data), img_dims, lfif->quality, output);
        break;

        case RGB48:
          return LFIFCompress<4, uint16_t, int32_t>(static_cast<const uint16_t *>(rgb_data), img_dims, lfif->quality, output);
        break;

        default:
          return -1;
        break;
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
  char     color_space [9] {};

  auto getCompressMethod = [](const string &magic_number) {
    if (magic_number == "LFIF-2D\n") return LFIF_2D;
    if (magic_number == "LFIF-3D\n") return LFIF_3D;
    if (magic_number == "LFIF-4D\n") return LFIF_4D;
    return LFIF_METHODS_CNT;
  };

  auto getColorSpace = [](const string &color_space) {
    if (color_space == "RGB24  \n") return RGB24;
    if (color_space == "RGB48  \n") return RGB48;
    return COLORSPACES_CNT;
  };

  cerr << "HELP\n";
  cerr << lfif->input_file_name << endl;

  input.open(lfif->input_file_name);
  if (input.fail()) {
    return -1;
  }

  input.read(magic_number, 8);
  lfif->method = getCompressMethod(string(magic_number));

  input.read(color_space, 8);
  lfif->color_space = getColorSpace(string(color_space));

  input.read(reinterpret_cast<char *>(&lfif->image_width), sizeof(lfif->image_width));
  lfif->image_width = be64toh(lfif->image_width);

  input.read(reinterpret_cast<char *>(&lfif->image_height), sizeof(lfif->image_height));
  lfif->image_height = be64toh(lfif->image_height);

  input.read(reinterpret_cast<char *>(&lfif->image_count), sizeof(lfif->image_count));
  lfif->image_count = be64toh(lfif->image_count);

  return 0;
}

int LFIFDecompress(LFIFDecompressStruct *lfif, void *rgb_buffer) {
  ifstream input       {};
  uint64_t img_dims[5] {};

  input.open(lfif->input_file_name);
  if (input.fail()) {
    return -1;
  }

  input.ignore(2 * 8 + 3 * 8);

  img_dims[0] = lfif->image_width;
  img_dims[1] = lfif->image_height;
  switch (lfif->method) {
    case LFIF_2D:
      img_dims[2] = lfif->image_count;
      switch (lfif->color_space) {
        case RGB24:
          return LFIFDecompress<2, uint8_t, int16_t>(input, img_dims, static_cast<uint8_t *>(rgb_buffer));
        break;

        case RGB48:
          return LFIFDecompress<2, uint16_t, int32_t>(input, img_dims, static_cast<uint16_t *>(rgb_buffer));
        break;

        default:
          return -1;
        break;
      }
    break;

    case LFIF_3D:
      img_dims[2] = lfif->image_count;
      img_dims[3] = 1;
      switch (lfif->color_space) {
        case RGB24:
          return LFIFDecompress<3, uint8_t, int16_t>(input, img_dims, static_cast<uint8_t *>(rgb_buffer));
        break;

        case RGB48:
          return LFIFDecompress<3, uint16_t, int32_t>(input, img_dims, static_cast<uint16_t *>(rgb_buffer));
        break;

        default:
          return -1;
        break;
      }
    break;

    case LFIF_4D:
      img_dims[2] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[3] = static_cast<uint64_t>(sqrt(lfif->image_count));
      img_dims[4] = 1;
      switch (lfif->color_space) {
        case RGB24:
          return LFIFDecompress<4, uint8_t, int16_t>(input, img_dims, static_cast<uint8_t *>(rgb_buffer));
        break;

        case RGB48:
          return LFIFDecompress<4, uint16_t, int32_t>(input, img_dims, static_cast<uint16_t *>(rgb_buffer));
        break;

        default:
          return -1;
        break;
      }
    break;

    default:
      return -1;
    break;
  }

  return -1;
}
