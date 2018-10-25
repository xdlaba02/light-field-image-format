/*******************************************************\
* SOUBOR: jpeg3d.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 21. 10. 2018
\*******************************************************/

#include "jpeg3d.h"
#include "endian.h"
#include "jpeg.h"
#include "jpeg_encoder.h"
#include "jpeg_decoder.h"

#include <algorithm>
#include <cstring>
#include <iostream>

bool RGBtoJPEG3D(const char *output_filename, const vector<uint8_t> &rgb_data, const uint64_t w, const uint64_t h, const uint64_t ix, const uint64_t iy, const uint8_t quality) {
  /*******************************************************\
  * Scale kvantizacni tabulky.
  \*******************************************************/
  QuantTable<3> quant_table {};
  scaleQuantTable<3>(quality, quant_table);

  uint64_t width = w;
  uint64_t height = h;
  uint64_t depth = ix * iy;

  cerr << "Width " << long(width) << endl;
  cerr << "Height " << long(height) << endl;
  cerr << "Depth " << long(depth) << endl;

  uint64_t blocks_width  = ceil(width/8.0);
  uint64_t blocks_height = ceil(height/8.0);
  uint64_t blocks_depth = ceil(depth/8.0);

  cerr << "Blocks w " << long(blocks_width) << endl;
  cerr << "Blocks h " << long(blocks_height) << endl;
  cerr << "Blocks d " << long(blocks_depth) << endl;

  uint64_t blocks_cnt = blocks_width * blocks_height * blocks_depth;

  int16_t prev_Y_DC  {};
  int16_t prev_Cb_DC {};
  int16_t prev_Cr_DC {};

  vector<int16_t>  Y_DC(blocks_cnt);
  vector<int16_t> Cb_DC(blocks_cnt);
  vector<int16_t> Cr_DC(blocks_cnt);

  vector<vector<RunLengthPair>>  Y_AC(blocks_cnt);
  vector<vector<RunLengthPair>> Cb_AC(blocks_cnt);
  vector<vector<RunLengthPair>> Cr_AC(blocks_cnt);

  map<uint8_t, uint64_t> weights_luma_AC   {};
  map<uint8_t, uint64_t> weights_luma_DC   {};
  map<uint8_t, uint64_t> weights_chroma_AC {};
  map<uint8_t, uint64_t> weights_chroma_DC {};

  /*******************************************************\
  * Vsechny bloky prevede AC koeficienty do podoby
    RunLength dvojic, DC koeficienty ulozi jako diference.
  * Do prislusnych map pocita county jednotlivych klicu,
    ktere se nasledne vyuziji pri huffmanove kodovani.
  \*******************************************************/

  for (uint64_t block_z = 0; block_z < blocks_depth; block_z++) {
    cerr << "Blocks index " << long(block_z*blocks_width*blocks_height) << " out of " << long(blocks_cnt) << endl;
    for (uint64_t block_y = 0; block_y < blocks_height; block_y++) {
      for (uint64_t block_x = 0; block_x < blocks_width; block_x++) {
        uint64_t block_index = block_z*blocks_width*blocks_height + block_y*blocks_width + block_x;

        Block<uint8_t, 3> block_Y_raw  {};
        Block<uint8_t, 3> block_Cb_raw {};
        Block<uint8_t, 3> block_Cr_raw {};

        for (uint8_t pixel_z = 0; pixel_z < 8; pixel_z++) {
          for (uint8_t pixel_y = 0; pixel_y < 8; pixel_y++) {
            for (uint8_t pixel_x = 0; pixel_x < 8; pixel_x++) {

              uint64_t image_x = block_x * 8 + pixel_x;
              uint64_t image_y = block_y * 8 + pixel_y;
              uint64_t image_z = block_z * 8 + pixel_z;

              /*******************************************************\
              * V pripade, ze velikost obrazku neni nasobek osmi,
                se blok doplni z krajnich pixelu.
              \*******************************************************/
              if (image_x > width - 1) {
                image_x = width - 1;
              }

              if (image_y > height - 1) {
                image_y = height - 1;
              }

              if (image_z > depth - 1) {
                image_z = depth - 1;
              }

              uint64_t input_pixel_index  = image_z*width*height + image_y*width + image_x;
              uint64_t output_pixel_index = pixel_z*8*8 + pixel_y*8 + pixel_x;

              uint8_t R = rgb_data[3 * input_pixel_index + 0];
              uint8_t G = rgb_data[3 * input_pixel_index + 1];
              uint8_t B = rgb_data[3 * input_pixel_index + 2];

              /*******************************************************\
              * Prevede z RGB na YCbCr.
              \*******************************************************/
              block_Y_raw[output_pixel_index]  =          0.299 * R +    0.587 * G +    0.114 * B;
              block_Cb_raw[output_pixel_index] = 128 - 0.168736 * R - 0.331264 * G +      0.5 * B;
              block_Cr_raw[output_pixel_index] = 128 +      0.5 * R - 0.418688 * G - 0.081312 * B;
            }
          }
        }

        Block<int16_t, 3> block_Y_zigzag  {};
        Block<int16_t, 3> block_Cb_zigzag {};
        Block<int16_t, 3> block_Cr_zigzag {};

        for (uint8_t w = 0; w < 8; w++) {
          for (uint8_t v = 0; v < 8; v++) {
            for (uint8_t u = 0; u < 8; u++) {
              uint16_t pixel_index = w*8*8 + v*8 + u;

              double sumY  = 0;
              double sumCb = 0;
              double sumCr = 0;

              for (uint8_t z = 0; z < 8; z++) {
                double cosc3 = cos(((2 * z + 1) * w * JPEG_PI ) / 16);

                for (uint8_t y = 0; y < 8; y++) {
                  double cosc2 = cos(((2 * y + 1) * v * JPEG_PI ) / 16);

                  for (uint8_t x = 0; x < 8; x++) {
                    double cosc1 = cos(((2 * x + 1) * u * JPEG_PI ) / 16);

                    uint16_t trans_pixel_index = z*8*8 + y*8 + x;

                    int8_t Y_val_shifted  = block_Y_raw[trans_pixel_index] - 128;
                    int8_t Cb_val_shifted = block_Cb_raw[trans_pixel_index] - 128;
                    int8_t Cr_val_shifted = block_Cr_raw[trans_pixel_index] - 128;

                    sumY  +=  Y_val_shifted * cosc1 * cosc2 * cosc3;
                    sumCb += Cb_val_shifted * cosc1 * cosc2 * cosc3;
                    sumCr += Cr_val_shifted * cosc1 * cosc2 * cosc3;
                  }
                }
              }

              double normU {u == 0 ? 1/sqrt(2) : 1};
              double normV {v == 0 ? 1/sqrt(2) : 1};
              double normW {w == 0 ? 1/sqrt(2) : 1};

              double invariant_coef = 0.25 * normU * normV * normW / quant_table[pixel_index];

              uint16_t zigzag_index = zigzagIndexTable<3>(pixel_index);

              block_Y_zigzag[zigzag_index]  = round(invariant_coef * sumY);
              block_Cb_zigzag[zigzag_index] = round(invariant_coef * sumCb);
              block_Cr_zigzag[zigzag_index] = round(invariant_coef * sumCr);
            }
          }
        }

        runLengthDiffEncode<3>(block_Y_zigzag, Y_DC[block_index], prev_Y_DC, Y_AC[block_index], weights_luma_DC, weights_luma_AC);
        runLengthDiffEncode<3>(block_Cb_zigzag, Cb_DC[block_index], prev_Cb_DC, Cb_AC[block_index], weights_chroma_DC, weights_chroma_AC);
        runLengthDiffEncode<3>(block_Cr_zigzag, Cr_DC[block_index], prev_Cr_DC, Cr_AC[block_index], weights_chroma_DC, weights_chroma_AC);
      }
    }
  }

  vector<pair<uint64_t, uint8_t>> codelengths_luma_DC   = huffmanGetCodelengths(weights_luma_DC);
  vector<pair<uint64_t, uint8_t>> codelengths_luma_AC   = huffmanGetCodelengths(weights_luma_AC);
  vector<pair<uint64_t, uint8_t>> codelengths_chroma_DC = huffmanGetCodelengths(weights_chroma_DC);
  vector<pair<uint64_t, uint8_t>> codelengths_chroma_AC = huffmanGetCodelengths(weights_chroma_AC);

  map<uint8_t, Codeword> huffcodes_luma_DC   = huffmanGenerateCodewords(codelengths_luma_DC);
  map<uint8_t, Codeword> huffcodes_luma_AC   = huffmanGenerateCodewords(codelengths_luma_AC);
  map<uint8_t, Codeword> huffcodes_chroma_DC = huffmanGenerateCodewords(codelengths_chroma_DC);
  map<uint8_t, Codeword> huffcodes_chroma_AC = huffmanGenerateCodewords(codelengths_chroma_AC);

  ofstream output(output_filename);
  if (output.fail()) {
    return false;
  }

  output.write("JPEG-3D\n", 8);

  uint64_t raw_w = toBigEndian(w);
  uint64_t raw_h = toBigEndian(h);
  uint64_t raw_ix = toBigEndian(ix);
  uint64_t raw_iy = toBigEndian(iy);

  output.write(reinterpret_cast<char *>(&raw_w), sizeof(uint64_t));
  output.write(reinterpret_cast<char *>(&raw_h), sizeof(uint64_t));
  output.write(reinterpret_cast<char *>(&raw_ix), sizeof(uint64_t));
  output.write(reinterpret_cast<char *>(&raw_iy), sizeof(uint64_t));

  output.write(reinterpret_cast<char *>(quant_table.data()), quant_table.size());
  output.write(reinterpret_cast<char *>(quant_table.data()), quant_table.size());

  writeHuffmanTable(codelengths_luma_DC, output);
  writeHuffmanTable(codelengths_luma_AC, output);
  writeHuffmanTable(codelengths_chroma_DC, output);
  writeHuffmanTable(codelengths_chroma_AC, output);

  OBitstream bitstream(output);

  for (uint64_t i = 0; i < blocks_cnt; i++) {
    encodeOnePair({0, Y_DC[i]}, huffcodes_luma_DC, bitstream);
    for (auto &pair: Y_AC[i]) {
      encodeOnePair(pair, huffcodes_luma_AC, bitstream);
    }

    encodeOnePair({0, Cb_DC[i]}, huffcodes_chroma_DC, bitstream);
    for (auto &pair: Cb_AC[i]) {
      encodeOnePair(pair, huffcodes_chroma_AC, bitstream);
    }

    encodeOnePair({0, Cr_DC[i]}, huffcodes_chroma_DC, bitstream);
    for (auto &pair: Cr_AC[i]) {
      encodeOnePair(pair, huffcodes_chroma_AC, bitstream);
    }
  }

  bitstream.flush();

  return true;
}


bool JPEG3DtoRGB(const char *input_filename, uint64_t &w, uint64_t &h, uint64_t &ix, uint64_t &iy, vector<uint8_t> &rgb_data) {
  ifstream input(input_filename);
  if (input.fail()) {
    return false;
  }

  char magic_number[8] {};

  input.read(magic_number, 8);
  if (strncmp(magic_number, "JPEG-3D\n", 8) != 0) {
    return false;
  }

  uint64_t raw_w  {};
  uint64_t raw_h  {};
  uint64_t raw_ix {};
  uint64_t raw_iy {};

  input.read(reinterpret_cast<char *>(&raw_w), sizeof(uint64_t));
  input.read(reinterpret_cast<char *>(&raw_h), sizeof(uint64_t));
  input.read(reinterpret_cast<char *>(&raw_ix), sizeof(uint64_t));
  input.read(reinterpret_cast<char *>(&raw_iy), sizeof(uint64_t));

  w  = fromBigEndian(raw_w);
  h = fromBigEndian(raw_h);
  ix = fromBigEndian(raw_ix);
  iy = fromBigEndian(raw_iy);

  uint64_t width = w;
  uint64_t height = h;
  uint64_t depth = ix * iy;

  QuantTable<3> quant_table_luma   {};
  QuantTable<3> quant_table_chroma {};

  input.read(reinterpret_cast<char *>(quant_table_luma.data()), quant_table_luma.size());
  input.read(reinterpret_cast<char *>(quant_table_chroma.data()), quant_table_luma.size());

  vector<uint8_t> huff_counts_luma_DC   {};
  vector<uint8_t> huff_counts_luma_AC   {};
  vector<uint8_t> huff_counts_chroma_DC {};
  vector<uint8_t> huff_counts_chroma_AC {};

  vector<uint8_t> huff_symbols_luma_DC   {};
  vector<uint8_t> huff_symbols_luma_AC   {};
  vector<uint8_t> huff_symbols_chroma_DC {};
  vector<uint8_t> huff_symbols_chroma_AC {};

  readHuffmanTable(huff_counts_luma_DC, huff_symbols_luma_DC, input);
  readHuffmanTable(huff_counts_luma_AC, huff_symbols_luma_AC, input);
  readHuffmanTable(huff_counts_chroma_DC, huff_symbols_chroma_DC, input);
  readHuffmanTable(huff_counts_chroma_AC, huff_symbols_chroma_AC, input);

  uint64_t blocks_width  = ceil(width/8.0);
  uint64_t blocks_height = ceil(height/8.0);
  uint64_t blocks_depth = ceil(depth/8.0);

  int16_t prev_Y_DC  = 0;
  int16_t prev_Cb_DC = 0;
  int16_t prev_Cr_DC = 0;

  rgb_data.resize(width * height * depth * 3);

  IBitstream bitstream(input);

  for (uint64_t block_z = 0; block_z < blocks_depth; block_z++) {
    cerr << "Blocks index " << long(block_z*blocks_width*blocks_height) << " out of " << long(blocks_width*blocks_height*blocks_depth) << endl;
    for (uint64_t block_y = 0; block_y < blocks_height; block_y++) {
      for (uint64_t block_x = 0; block_x < blocks_width; block_x++) {

        Block<int16_t, 3> block_Y  {};
        Block<int16_t, 3> block_Cb {};
        Block<int16_t, 3> block_Cr {};

        readOneBlock<3>(huff_counts_luma_DC, huff_symbols_luma_DC, huff_counts_luma_AC, huff_symbols_luma_AC, quant_table_luma, prev_Y_DC, block_Y, bitstream);
        readOneBlock<3>(huff_counts_chroma_DC, huff_symbols_chroma_DC, huff_counts_chroma_AC, huff_symbols_chroma_AC, quant_table_chroma, prev_Cb_DC, block_Cb, bitstream);
        readOneBlock<3>(huff_counts_chroma_DC, huff_symbols_chroma_DC, huff_counts_chroma_AC, huff_symbols_chroma_AC, quant_table_chroma, prev_Cr_DC, block_Cr, bitstream);

        for (uint8_t z = 0; z < 8; z++) {
          for (uint8_t y = 0; y < 8; y++) {
            for (uint8_t x = 0; x < 8; x++) {

              double sumY  = 0;
              double sumCb = 0;
              double sumCr = 0;

              for (uint8_t w = 0; w < 8; w++) {
                double cosc3 = cos(((2 * z + 1) * w * JPEG_PI) / 16);

                for (uint8_t v = 0; v < 8; v++) {
                  double cosc2 = cos(((2 * y + 1) * v * JPEG_PI) / 16);

                  for (uint8_t u = 0; u < 8; u++) {
                    double cosc1 = cos(((2 * x + 1) * u * JPEG_PI) / 16);


                    uint16_t trans_pixel_index = w*8*8 + v*8 + u;
                    double normU = (u == 0 ? 1/sqrt(2) : 1);
                    double normV = (v == 0 ? 1/sqrt(2) : 1);
                    double normW = (w == 0 ? 1/sqrt(2) : 1);

                    sumY += block_Y[trans_pixel_index] * normU * normV * normW * cosc1 * cosc2 * cosc3;
                    sumCb += block_Cb[trans_pixel_index] * normU * normV * normW * cosc1 * cosc2 * cosc3;
                    sumCr += block_Cr[trans_pixel_index] * normU * normV * normW * cosc1 * cosc2 * cosc3;
                  }
                }
              }

              uint8_t Y  = clamp((0.25 * sumY) + 128, 0.0, 255.0);
              uint8_t Cb = clamp((0.25 * sumCb) + 128, 0.0, 255.0);
              uint8_t Cr = clamp((0.25 * sumCr) + 128, 0.0, 255.0);

              uint64_t real_pixel_x = block_x*8 + x;
              if (real_pixel_x >= width) {
                continue;
              }

              uint64_t real_pixel_y = block_y*8 + y;
              if (real_pixel_y >= height) {
                continue;
              }

              uint64_t real_pixel_z = block_z*8 + z;
              if (real_pixel_z >= depth) {
                continue;
              }

              uint64_t real_pixel_index = real_pixel_z*width*height + real_pixel_y*width + real_pixel_x;

              rgb_data[3 * real_pixel_index + 0] = clamp(Y + 1.402                            * (Cr - 128), 0.0, 255.0);
              rgb_data[3 * real_pixel_index + 1] = clamp(Y - 0.344136 * (Cb - 128) - 0.714136 * (Cr - 128), 0.0, 255.0);
              rgb_data[3 * real_pixel_index + 2] = clamp(Y + 1.772    * (Cb - 128)                        , 0.0, 255.0);
            }
          }
        }
      }
    }
  }

  return true;
}
