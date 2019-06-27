/******************************************************************************\
* SOUBOR: center_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "compress.h"
#include "plenoppm.h"

#include <colorspace.h>
#include <lfif_encoder.h>
#include <lfif_decoder.h>

#include <cmath>

#include <iostream>
#include <vector>

#ifdef BLOCK_SIZE
const size_t BS = BLOCK_SIZE;
#else
const size_t BS = 8;
#endif

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_file_mask   {};
  const char *output_file_name  {};
  float quality                 {};

  vector<uint8_t> rgb_data      {};
  vector<uint8_t> prediction    {};

  uint64_t width                {};
  uint64_t height               {};
  uint64_t image_count          {};
  uint32_t max_rgb_value        {};

  LfifEncoder<BS, 2> *encoder2D {};
  LfifDecoder<BS, 2> *decoder2D {};
  ifstream            input     {};

  LfifEncoder<BS, 4> *encoder4D {};
  ofstream            output    {};

  if (!parse_args(argc, argv, input_file_mask, output_file_name, quality)) {
    return 1;
  }

  if (!checkPPMheaders(input_file_mask, width, height, max_rgb_value, image_count)) {
    return 2;
  }

  size_t view_size = width * height * 3;
  view_size *= (max_rgb_value > 255) ? 2 : 1;
  size_t image_size = view_size * image_count;

  rgb_data.resize(image_size);
  prediction.resize(view_size);

  cerr << prediction.size() << endl;

  cerr << "INFO: LOADING IMAGE" << endl;

  if (!loadPPMs(input_file_mask, rgb_data.data())) {
    return 2;
  }

  size_t last_slash_pos = string(output_file_name).find_last_of('/');
  if (last_slash_pos != string::npos) {
    string command = "mkdir -p " + string(output_file_name).substr(0, last_slash_pos);
    system(command.c_str());
  }

  output.open(output_file_name, ios::binary);
  if (!output) {
    cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }

  encoder2D = new LfifEncoder<BS, 2>;
  decoder2D = new LfifDecoder<BS, 2>;
  encoder4D = new LfifEncoder<BS, 4>;

  cerr << "INFO: ENCODING CENTER VIEW" << endl;

  encoder2D->img_dims[0] = width;
  encoder2D->img_dims[1] = height;
  encoder2D->img_dims[2] = 1;
  encoder2D->color_depth = ceil(log2(max_rgb_value + 1));

  auto inputF02D = [&](size_t channel, size_t index) -> RGBUNIT {
    size_t center_image_index = image_count / 2;
    if (max_rgb_value < 256) {
      return reinterpret_cast<const uint8_t *>(rgb_data.data())[(center_image_index * width * height + index) * 3 + channel];
    }
    else {
      return reinterpret_cast<const uint16_t *>(rgb_data.data())[(center_image_index * width * height + index) * 3 + channel];
    }
  };

  auto inputF2D = [&](size_t index) -> INPUTTRIPLET {
    RGBUNIT R = inputF02D(0, index);
    RGBUNIT G = inputF02D(1, index);
    RGBUNIT B = inputF02D(2, index);

    INPUTUNIT  Y = YCbCr::RGBToY(R, G, B) - (max_rgb_value + 1) / 2;
    INPUTUNIT Cb = YCbCr::RGBToCb(R, G, B);
    INPUTUNIT Cr = YCbCr::RGBToCr(R, G, B);

    return {Y, Cb, Cr};
  };

  initEncoder(*encoder2D);
  constructQuantizationTables(*encoder2D, "DEFAULT", quality);
  constructTraversalTables(*encoder2D, "DEFAULT");
  huffmanScan(*encoder2D, inputF2D);
  constructHuffmanTables(*encoder2D);
  writeHeader(*encoder2D, output);
  outputScan(*encoder2D, inputF2D, output);

  output.flush();


  cerr << "INFO: DECODING CENTER VIEW" << endl;

  input.open(output_file_name, ios::binary);
  if (!input) {
    cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR READING\n";
    return 1;
  }

  if (readHeader(*decoder2D, input)) {
    cerr << "ERROR: IMAGE HEADER INVALID\n";
    return 2;
  }

  initDecoder(*decoder2D);

  auto outputF02D = [&](size_t channel, size_t index, RGBUNIT value) {
    if (decoder2D->color_depth > 8) {
      reinterpret_cast<uint16_t *>(prediction.data())[index * 3 + channel] = value;
    }
    else {
      reinterpret_cast<uint8_t *>(prediction.data())[index * 3 + channel] = value;
    }
  };

  auto outputF2D = [&](size_t index, const INPUTTRIPLET &triplet) {
    INPUTUNIT  Y = triplet[0] + ((max_rgb_value + 1) / 2);
    INPUTUNIT Cb = triplet[1];
    INPUTUNIT Cr = triplet[2];

    RGBUNIT R = clamp<INPUTUNIT>(round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, max_rgb_value);
    RGBUNIT G = clamp<INPUTUNIT>(round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, max_rgb_value);
    RGBUNIT B = clamp<INPUTUNIT>(round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, max_rgb_value);

    outputF02D(0, index, R);
    outputF02D(1, index, G);
    outputF02D(2, index, B);
  };

  decodeScan(*decoder2D, input, outputF2D);

  cerr << "INFO: ENCODING RESIDUUM" << endl;

  encoder4D->img_dims[0] = width;
  encoder4D->img_dims[1] = height;
  encoder4D->img_dims[2] = sqrt(image_count);
  encoder4D->img_dims[3] = sqrt(image_count);
  encoder4D->img_dims[4] = 1;
  encoder4D->color_depth = ceil(log2(max_rgb_value + 1)) + 1;

  auto inputF0 = [&](size_t channel, size_t index) -> INPUTUNIT {
    if (max_rgb_value < 256) {
      return static_cast<INPUTUNIT>(reinterpret_cast<const uint8_t *>(rgb_data.data())[index * 3 + channel])   - reinterpret_cast<const uint8_t *>(prediction.data())[(index % (width * height)) * 3 + channel] + max_rgb_value;
    }
    else {
      return static_cast<INPUTUNIT>(reinterpret_cast<const uint16_t *>(rgb_data.data())[index * 3 + channel])  - reinterpret_cast<const uint16_t *>(prediction.data())[(index % (width * height)) * 3 + channel] + max_rgb_value;
    }
  };

  auto inputF = [&](size_t index) -> INPUTTRIPLET {
    INPUTUNIT R = inputF0(0, index);
    INPUTUNIT G = inputF0(1, index);
    INPUTUNIT B = inputF0(2, index);

    INPUTUNIT  Y = YCbCr::RGBToY(R, G, B) - max_rgb_value + 1;
    INPUTUNIT Cb = YCbCr::RGBToCb(R, G, B);
    INPUTUNIT Cr = YCbCr::RGBToCr(R, G, B);

    return {Y, Cb, Cr};
  };

  initEncoder(*encoder4D);
  constructQuantizationTables(*encoder4D, "DEFAULT", quality);
  constructTraversalTables(*encoder4D, "DEFAULT");
  huffmanScan(*encoder4D, inputF); //SECOND IMAGE SCAN
  constructHuffmanTables(*encoder4D);
  writeHeader(*encoder4D, output);
  outputScan(*encoder4D, inputF, output); //THIRD IMAGE SCAN

  delete encoder4D;
  delete decoder2D;
  delete encoder2D;

  return 0;
}
