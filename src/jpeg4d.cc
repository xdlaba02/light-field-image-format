/*******************************************************\
* SOUBOR: jpeg4d.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 26. 10. 2018
\*******************************************************/

#include "jpeg4d.h"
#include "endian.h"
#include "jpeg.h"
#include "jpeg_encoder.h"
#include "jpeg_decoder.h"
#include "dct.h"

#include <cstring>
#include <ctime>

#include <iostream>
#include <iomanip>

#include <algorithm>

bool RGBtoJPEG4D(const char *output_filename, const vector<uint8_t> &rgb_data, const uint64_t w, const uint64_t h, const uint64_t ix, const uint64_t iy, const uint8_t quality) {
  clock_t clock_start {};
  cerr << fixed << setprecision(3);

  /*******************************************************\
  * Scale kvantizacni tabulky.
  \*******************************************************/
  QuantTable<4> quant_table {};

  cerr << "CONSTRUCTING QUANTIZATION TABLE" << endl;
  clock_start = clock();

  // teoreticky by se dala generovat optimalni kvantizacni tabulka z jiz vygenerovanych koeficientu
  constructQuantTable<4>(quality, quant_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  ZigzagTable<4> zigzag_table {};

  cerr << "CONSTRUCTING ZIGZAG TABLE" << endl;
  clock_start = clock();

  constructZigzagTable<4>(quant_table, zigzag_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  uint64_t width = w;
  uint64_t height = h;
  uint64_t images_x = ix;
  uint64_t images_y = iy;

  uint64_t blocks_width  = ceil(width/8.0);
  uint64_t blocks_height = ceil(height/8.0);
  uint64_t blocks_images_x = ceil(images_x/8.0);
  uint64_t blocks_images_y = ceil(images_y/8.0);

  uint64_t blocks_cnt = blocks_width * blocks_height * blocks_images_x * blocks_images_y;

  int16_t prev_Y_DC  {};
  int16_t prev_Cb_DC {};
  int16_t prev_Cr_DC {};

  vector<int16_t>  Y_DC(blocks_cnt);
  vector<int16_t> Cb_DC(blocks_cnt);
  vector<int16_t> Cr_DC(blocks_cnt);

  vector<vector<RunLengthPair>>  Y_AC(blocks_cnt);
  vector<vector<RunLengthPair>> Cb_AC(blocks_cnt);
  vector<vector<RunLengthPair>> Cr_AC(blocks_cnt);

  cerr << "TRANSFORMING AND RUNLENGHT ENCODING BLOCKS" << endl;
  clock_start = clock();

  /*******************************************************\
  * Vsechny bloky prevede AC koeficienty do podoby
    RunLength dvojic, DC koeficienty ulozi jako diference.
  * Do prislusnych map pocita county jednotlivych klicu,
    ktere se nasledne vyuziji pri huffmanove kodovani.
  \*******************************************************/
  for (uint64_t block_iy = 0; block_iy < blocks_images_y; block_iy++) {
    for (uint64_t block_ix = 0; block_ix < blocks_images_x; block_ix++) {
      for (uint64_t block_y = 0; block_y < blocks_height; block_y++) {
        for (uint64_t block_x = 0; block_x < blocks_width; block_x++) {
          uint64_t block_index = block_iy*blocks_width*blocks_height*blocks_images_x + block_ix*blocks_width*blocks_height + block_y*blocks_width + block_x;

          if (!(block_index % (blocks_cnt/50))) {
            cerr << "#";
          }

          Block<uint8_t, 4> block_R  {};
          Block<uint8_t, 4> block_G  {};
          Block<uint8_t, 4> block_B  {};

          for (uint8_t pixel_iy = 0; pixel_iy < 8; pixel_iy++) {
            for (uint8_t pixel_ix = 0; pixel_ix < 8; pixel_ix++) {
              for (uint8_t pixel_y = 0; pixel_y < 8; pixel_y++) {
                for (uint8_t pixel_x = 0; pixel_x < 8; pixel_x++) {
                  uint16_t pixel_index = pixel_iy*8*8*8 + pixel_ix*8*8 + pixel_y*8 + pixel_x;

                  uint64_t image_x = block_x * 8 + pixel_x;
                  uint64_t image_y = block_y * 8 + pixel_y;
                  uint64_t image_ix = block_ix * 8 + pixel_ix;
                  uint64_t image_iy = block_iy * 8 + pixel_iy;

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

                  if (image_ix > images_x - 1) {
                    image_ix = images_x - 1;
                  }

                  if (image_iy > images_y - 1) {
                    image_iy = images_y - 1;
}

                  uint64_t input_pixel_index = image_iy*width*height*images_x + image_ix*width*height + image_y*width + image_x;

                  block_R[pixel_index] = rgb_data[3 * input_pixel_index + 0];
                  block_G[pixel_index] = rgb_data[3 * input_pixel_index + 1];
                  block_B[pixel_index] = rgb_data[3 * input_pixel_index + 2];
                }
              }
            }
          }

          Block<uint8_t, 4> block_Y  {};
          Block<uint8_t, 4> block_Cb {};
          Block<uint8_t, 4> block_Cr {};

          /*******************************************************\
          * Konvertuje RGB na YCbCr
          \*******************************************************/
          for (uint16_t pixel_index = 0; pixel_index < 8*8*8*8; pixel_index++) {
            uint8_t R = block_R[pixel_index];
            uint8_t G = block_G[pixel_index];
            uint8_t B = block_B[pixel_index];

            block_Y[pixel_index]  =          0.299 * R +    0.587 * G +    0.114 * B;
            block_Cb[pixel_index] = 128 - 0.168736 * R - 0.331264 * G +      0.5 * B;
            block_Cr[pixel_index] = 128 +      0.5 * R - 0.418688 * G - 0.081312 * B;
          }

          Block<int8_t, 4> block_Y_shifted  {};
          Block<int8_t, 4> block_Cb_shifted {};
          Block<int8_t, 4> block_Cr_shifted {};

          for (uint16_t pixel_index = 0; pixel_index < 8*8*8*8; pixel_index++) {
            block_Y_shifted[pixel_index]  = block_Y[pixel_index]  - 128;
            block_Cb_shifted[pixel_index] = block_Cb[pixel_index] - 128;
            block_Cr_shifted[pixel_index] = block_Cr[pixel_index] - 128;
          }

          Block<float, 4> block_Y_transformed  {};
          Block<float, 4> block_Cb_transformed {};
          Block<float, 4> block_Cr_transformed {};

          auto fdct = [](Block<int8_t, 4> &block_shifted, Block<float, 4> &block_transformed) {
            fdct4([&](uint8_t x, uint8_t y, uint8_t ix, uint8_t iy){ return block_shifted[iy*8*8*8 + ix*8*8 + y*8 + x]; }, [&](uint8_t x, uint8_t y, uint8_t ix, uint8_t iy) -> float & { return block_transformed[iy*8*8*8 + ix*8*8 + y*8 + x]; });
          };

          fdct(block_Y_shifted,  block_Y_transformed);
          fdct(block_Cb_shifted, block_Cb_transformed);
          fdct(block_Cr_shifted, block_Cr_transformed);

          Block<int16_t, 4> block_Y_quantized  {};
          Block<int16_t, 4> block_Cb_quantized {};
          Block<int16_t, 4> block_Cr_quantized {};

          for (uint16_t pixel_index = 0; pixel_index < 8*8*8*8; pixel_index++) {
            block_Y_quantized[pixel_index]  = block_Y_transformed[pixel_index]  * 0.0625 / quant_table[pixel_index];
            block_Cb_quantized[pixel_index] = block_Cb_transformed[pixel_index] * 0.0625 / quant_table[pixel_index];
            block_Cr_quantized[pixel_index] = block_Cr_transformed[pixel_index] * 0.0625 / quant_table[pixel_index];
          }

          Block<int16_t, 4> block_Y_zigzag  {};
          Block<int16_t, 4> block_Cb_zigzag {};
          Block<int16_t, 4> block_Cr_zigzag {};

          for (uint16_t pixel_index = 0; pixel_index < 8*8*8*8; pixel_index++) {
            uint16_t zigzag_index = zigzag_table[pixel_index];
            block_Y_zigzag[zigzag_index]  = block_Y_quantized[pixel_index];
            block_Cb_zigzag[zigzag_index] = block_Cb_quantized[pixel_index];
            block_Cr_zigzag[zigzag_index] = block_Cr_quantized[pixel_index];
          }

          runLengthDiffEncode<4>(block_Y_zigzag,  Y_DC[block_index],  Y_AC[block_index],  prev_Y_DC);
          runLengthDiffEncode<4>(block_Cb_zigzag, Cb_DC[block_index], Cb_AC[block_index], prev_Cb_DC);
          runLengthDiffEncode<4>(block_Cr_zigzag, Cr_DC[block_index], Cr_AC[block_index], prev_Cr_DC);
        }
      }
    }
  }

  cerr << " ";

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  map<uint8_t, uint64_t> weights_luma_DC   {};
  map<uint8_t, uint64_t> weights_chroma_DC {};
  map<uint8_t, uint64_t> weights_luma_AC   {};
  map<uint8_t, uint64_t> weights_chroma_AC {};

  cerr << "COUNTING RUNLENGTH PAIR WEIGHTS" << endl;
  clock_start = clock();

  huffmanGetWeightsDC(Y_DC,  weights_luma_DC);
  huffmanGetWeightsDC(Cb_DC, weights_chroma_DC);
  huffmanGetWeightsDC(Cr_DC, weights_chroma_DC);

  huffmanGetWeightsAC(Y_AC,  weights_luma_AC);
  huffmanGetWeightsAC(Cb_AC, weights_chroma_AC);
  huffmanGetWeightsAC(Cr_AC, weights_chroma_AC);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  cerr << "GENERATING HUFFMAN CODE LENGTHS" << endl;
  clock_start = clock();

  vector<pair<uint64_t, uint8_t>> codelengths_luma_DC   = huffmanGetCodelengths(weights_luma_DC);
  vector<pair<uint64_t, uint8_t>> codelengths_luma_AC   = huffmanGetCodelengths(weights_luma_AC);
  vector<pair<uint64_t, uint8_t>> codelengths_chroma_DC = huffmanGetCodelengths(weights_chroma_DC);
  vector<pair<uint64_t, uint8_t>> codelengths_chroma_AC = huffmanGetCodelengths(weights_chroma_AC);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  cerr << "GENERATING HUFFMAN CODEWORDS" << endl;
  clock_start = clock();

  map<uint8_t, Codeword> huffcodes_luma_DC   = huffmanGenerateCodewords(codelengths_luma_DC);
  map<uint8_t, Codeword> huffcodes_luma_AC   = huffmanGenerateCodewords(codelengths_luma_AC);
  map<uint8_t, Codeword> huffcodes_chroma_DC = huffmanGenerateCodewords(codelengths_chroma_DC);
  map<uint8_t, Codeword> huffcodes_chroma_AC = huffmanGenerateCodewords(codelengths_chroma_AC);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  cerr << "OPENING OUTPUT FILE TO WRITE" << endl;
  clock_start = clock();

  ofstream output(output_filename);
  if (output.fail()) {
    return false;
  }

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  cerr << "WRITING MAGIC NUMBER" << endl;
  clock_start = clock();

  output.write("JPEG-4D\n", 8);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  uint64_t raw_w = toBigEndian(w);
  uint64_t raw_h = toBigEndian(h);
  uint64_t raw_ix = toBigEndian(ix);
  uint64_t raw_iy = toBigEndian(iy);

  cerr << "WRITING IMAGE DIMENSIONS" << endl;
  clock_start = clock();

  output.write(reinterpret_cast<char *>(&raw_w), sizeof(uint64_t));
  output.write(reinterpret_cast<char *>(&raw_h), sizeof(uint64_t));
  output.write(reinterpret_cast<char *>(&raw_ix), sizeof(uint64_t));
  output.write(reinterpret_cast<char *>(&raw_iy), sizeof(uint64_t));

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  cerr << "WRITING QUANTIZATION TABLE" << endl;
  clock_start = clock();

  output.write(reinterpret_cast<char *>(quant_table.data()), quant_table.size());

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  cerr << "WRITING HUFFMAN TABLES" << endl;
  clock_start = clock();

  writeHuffmanTable(codelengths_luma_DC, output);
  writeHuffmanTable(codelengths_luma_AC, output);
  writeHuffmanTable(codelengths_chroma_DC, output);
  writeHuffmanTable(codelengths_chroma_AC, output);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  OBitstream bitstream(output);

  cerr << "HUFFMAN ENCODING AND WRITING BLOCKS" << endl;
  clock_start = clock();

  for (uint64_t i = 0; i < blocks_cnt; i++) {
    writeOneBlock(Y_DC[i],  Y_AC[i],  huffcodes_luma_DC,   huffcodes_luma_AC,   bitstream);
    writeOneBlock(Cb_DC[i], Cb_AC[i], huffcodes_chroma_DC, huffcodes_chroma_AC, bitstream);
    writeOneBlock(Cr_DC[i], Cr_AC[i], huffcodes_chroma_DC, huffcodes_chroma_AC, bitstream);
  }

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  cerr << "FLUSHING OUTPUT" << endl;
  clock_start = clock();

  bitstream.flush();

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  return true;
}






bool JPEG4DtoRGB(const char *input_filename, uint64_t &w, uint64_t &h, uint64_t &ix, uint64_t &iy, vector<uint8_t> &rgb_data) {
  clock_t clock_start {};
  cerr << fixed << setprecision(3);

  cerr << "OPENING INPUT FILE" << endl;
  clock_start = clock();

  ifstream input(input_filename);
  if (input.fail()) {
    return false;
  }

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  cerr << "READING MAGIC NUMBER" << endl;
  clock_start = clock();

  char magic_number[8] {};

  input.read(magic_number, 8);
  if (strncmp(magic_number, "JPEG-4D\n", 8) != 0) {
    return false;
  }

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  uint64_t raw_w  {};
  uint64_t raw_h  {};
  uint64_t raw_ix {};
  uint64_t raw_iy {};

  cerr << "READING IMAGE DIMENSIONS" << endl;
  clock_start = clock();

  input.read(reinterpret_cast<char *>(&raw_w), sizeof(uint64_t));
  input.read(reinterpret_cast<char *>(&raw_h), sizeof(uint64_t));
  input.read(reinterpret_cast<char *>(&raw_ix), sizeof(uint64_t));
  input.read(reinterpret_cast<char *>(&raw_iy), sizeof(uint64_t));

  w  = fromBigEndian(raw_w);
  h = fromBigEndian(raw_h);
  ix = fromBigEndian(raw_ix);
  iy = fromBigEndian(raw_iy);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  uint64_t width = w;
  uint64_t height = h;
  uint64_t images_x = ix;
  uint64_t images_y = iy;

  QuantTable<4> quant_table   {};

  cerr << "READING QUANTIZATION TABLES" << endl;
  clock_start = clock();

  input.read(reinterpret_cast<char *>(quant_table.data()), quant_table.size());

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  ZigzagTable<4> zigzag_table {};

  cerr << "CONSTRUCTING ZIGZAG TABLE" << endl;
  clock_start = clock();

  constructZigzagTable<4>(quant_table, zigzag_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  vector<uint8_t> huff_counts_luma_DC   {};
  vector<uint8_t> huff_counts_luma_AC   {};
  vector<uint8_t> huff_counts_chroma_DC {};
  vector<uint8_t> huff_counts_chroma_AC {};

  vector<uint8_t> huff_symbols_luma_DC   {};
  vector<uint8_t> huff_symbols_luma_AC   {};
  vector<uint8_t> huff_symbols_chroma_DC {};
  vector<uint8_t> huff_symbols_chroma_AC {};

  cerr << "READING HUFFMAN TABLES" << endl;
  clock_start = clock();

  readHuffmanTable(huff_counts_luma_DC, huff_symbols_luma_DC, input);
  readHuffmanTable(huff_counts_luma_AC, huff_symbols_luma_AC, input);
  readHuffmanTable(huff_counts_chroma_DC, huff_symbols_chroma_DC, input);
  readHuffmanTable(huff_counts_chroma_AC, huff_symbols_chroma_AC, input);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  uint64_t blocks_width  = ceil(width/8.0);
  uint64_t blocks_height = ceil(height/8.0);
  uint64_t blocks_images_x = ceil(images_x/8.0);
  uint64_t blocks_images_y = ceil(images_y/8.0);

  uint64_t blocks_cnt = blocks_width * blocks_height * blocks_images_x * blocks_images_y;

  int16_t prev_Y_DC  = 0;
  int16_t prev_Cb_DC = 0;
  int16_t prev_Cr_DC = 0;

  cerr << "RESIZING RGB BUFFER" << endl;
  clock_start = clock();

  rgb_data.resize(width * height * images_x * images_y * 3);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  IBitstream bitstream(input);

  cerr << "DECODING BLOCKS" << endl;
  clock_start = clock();

  for (uint64_t block_iy = 0; block_iy < blocks_images_y; block_iy++) {
    for (uint64_t block_ix = 0; block_ix < blocks_images_x; block_ix++) {
      for (uint64_t block_y = 0; block_y < blocks_height; block_y++) {
        for (uint64_t block_x = 0; block_x < blocks_width; block_x++) {
          uint64_t block_index = block_iy*blocks_width*blocks_height*blocks_images_x + block_ix*blocks_width*blocks_height + block_y*blocks_width + block_x;

          if (!(block_index % (blocks_cnt/50))) {
            cerr << "#";
          }

          int16_t Y_DC  {};
          int16_t Cb_DC {};
          int16_t Cr_DC {};

          vector<RunLengthPair>  Y_AC {};
          vector<RunLengthPair> Cb_AC {};
          vector<RunLengthPair> Cr_AC {};

          readOneBlock(huff_counts_luma_DC,   huff_counts_luma_AC,   huff_symbols_luma_DC,   huff_symbols_luma_AC,   Y_DC,  Y_AC,  bitstream);
          readOneBlock(huff_counts_chroma_DC, huff_counts_chroma_AC, huff_symbols_chroma_DC, huff_symbols_chroma_AC, Cb_DC, Cb_AC, bitstream);
          readOneBlock(huff_counts_chroma_DC, huff_counts_chroma_AC, huff_symbols_chroma_DC, huff_symbols_chroma_AC, Cr_DC, Cr_AC, bitstream);

          Block<int16_t, 4> block_Y_raw  {};
          Block<int16_t, 4> block_Cb_raw {};
          Block<int16_t, 4> block_Cr_raw {};

          runLengthDiffDecode<4>(Y_DC,  Y_AC,  prev_Y_DC,  block_Y_raw);
          runLengthDiffDecode<4>(Cb_DC, Cb_AC, prev_Cb_DC, block_Cb_raw);
          runLengthDiffDecode<4>(Cr_DC, Cr_AC, prev_Cr_DC, block_Cr_raw);

          Block<int16_t, 4> block_Y_dezigzaged  {};
          Block<int16_t, 4> block_Cb_dezigzaged {};
          Block<int16_t, 4> block_Cr_dezigzaged {};

          for (uint16_t pixel_index = 0; pixel_index < 8*8*8*8; pixel_index++) {
            uint16_t zigzag_index = zigzag_table[pixel_index];
            block_Y_dezigzaged[pixel_index]  = block_Y_raw[zigzag_index];
            block_Cb_dezigzaged[pixel_index] = block_Cb_raw[zigzag_index];
            block_Cr_dezigzaged[pixel_index] = block_Cr_raw[zigzag_index];
          }

          Block<float, 4> block_Y_dequantized  {};
          Block<float, 4> block_Cb_dequantized {};
          Block<float, 4> block_Cr_dequantized {};

          for (uint16_t pixel_index = 0; pixel_index < 8*8*8*8; pixel_index++) {
            block_Y_dequantized[pixel_index]  = block_Y_dezigzaged[pixel_index]  * 0.0625 * quant_table[pixel_index];
            block_Cb_dequantized[pixel_index] = block_Cb_dezigzaged[pixel_index] * 0.0625 * quant_table[pixel_index];
            block_Cr_dequantized[pixel_index] = block_Cr_dezigzaged[pixel_index] * 0.0625 * quant_table[pixel_index];
          }

          Block<float, 4> block_Y_detransformed  {};
          Block<float, 4> block_Cb_detransformed {};
          Block<float, 4> block_Cr_detransformed {};

          auto idct = [](Block<float, 4> &block_dequantized, Block<float, 4> &block_detransformed) {
            idct4([&](uint8_t x, uint8_t y, uint8_t ix, uint8_t iy){ return block_dequantized[iy*8*8*8 + ix*8*8 + y*8 + x]; }, [&](uint8_t x, uint8_t y, uint8_t ix, uint8_t iy) -> float & { return block_detransformed[iy*8*8*8 + ix*8*8 + y*8 + x]; });
          };

          idct(block_Y_dequantized, block_Y_detransformed);
          idct(block_Cb_dequantized, block_Cb_detransformed);
          idct(block_Cr_dequantized, block_Cr_detransformed);

          Block<float, 4> block_Y_deshifted  {};
          Block<float, 4> block_Cb_deshifted {};
          Block<float, 4> block_Cr_deshifted {};

          for (uint16_t pixel_index = 0; pixel_index < 8*8*8*8; pixel_index++) {
            block_Y_deshifted[pixel_index]  = block_Y_detransformed[pixel_index]  + 128;
            block_Cb_deshifted[pixel_index] = block_Cb_detransformed[pixel_index] + 128;
            block_Cr_deshifted[pixel_index] = block_Cr_detransformed[pixel_index] + 128;
          }

          Block<uint8_t, 4> block_R {};
          Block<uint8_t, 4> block_G {};
          Block<uint8_t, 4> block_B {};

          /*******************************************************\
          * Konvertuje RGB na YCbCr
          \*******************************************************/
          for (uint16_t pixel_index = 0; pixel_index < 8*8*8*8; pixel_index++) {
            float Y  = block_Y_deshifted[pixel_index];
            float Cb = block_Cb_deshifted[pixel_index];
            float Cr = block_Cr_deshifted[pixel_index];

            block_R[pixel_index] = clamp(Y + 1.402                            * (Cr - 128), 0.0, 255.0);
            block_G[pixel_index] = clamp(Y - 0.344136 * (Cb - 128) - 0.714136 * (Cr - 128), 0.0, 255.0);
            block_B[pixel_index] = clamp(Y + 1.772    * (Cb - 128)                        , 0.0, 255.0);
          }

          for (uint8_t pixel_iy = 0; pixel_iy < 8; pixel_iy++) {
            for (uint8_t pixel_ix = 0; pixel_ix < 8; pixel_ix++) {
              for (uint8_t pixel_y = 0; pixel_y < 8; pixel_y++) {
                for (uint8_t pixel_x = 0; pixel_x < 8; pixel_x++) {
                  uint16_t pixel_index = pixel_iy*8*8*8 + pixel_ix*8*8 + pixel_y*8 + pixel_x;

                  uint64_t real_pixel_x = block_x*8 + pixel_x;
                  if (real_pixel_x >= width) {
                    continue;
                  }

                  uint64_t real_pixel_y = block_y*8 + pixel_y;
                  if (real_pixel_y >= height) {
                    continue;
                  }

                  uint64_t real_pixel_ix = block_ix*8 + pixel_ix;
                  if (real_pixel_ix >= images_x) {
                    continue;
                  }

                  uint64_t real_pixel_iy = block_iy*8 + pixel_iy;
                  if (real_pixel_iy >= images_y) {
                    continue;
                  }

                  uint64_t real_pixel_index = real_pixel_iy*width*height*images_x + real_pixel_ix*width*height + real_pixel_y*width + real_pixel_x;

                  rgb_data[3 * real_pixel_index + 0] = block_R[pixel_index];
                  rgb_data[3 * real_pixel_index + 1] = block_G[pixel_index];
                  rgb_data[3 * real_pixel_index + 2] = block_B[pixel_index];
                }
              }
            }
          }
        }
      }
    }
  }

  cerr << " ";

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  return true;
}
