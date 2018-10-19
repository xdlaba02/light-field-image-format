/*******************************************************\
* SOUBOR: jpeg2d_encoder.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "jpeg2d.h"
#include "ppm.h"

#include <bitset>
#include <fstream>
#include <algorithm>

bool PPM2JPEG2D(const char *input_filename, const char *output_filename, const uint8_t quality) {
  vector<uint8_t> rgb_data {};
  uint64_t width       {};
  uint64_t height      {};

  if (!loadPPM(input_filename, width, height, rgb_data)) {
    return false;
  }

  /*******************************************************\
  * Scale kvantizacni tabulky.
  \*******************************************************/
  QuantTable quant_table {};
  double scaled_coef     {};
  if (quality < 50) {
    scaled_coef = (5000.0 / quality) / 100;
  }
  else {
    scaled_coef = (200.0 - 2 * quality) / 100;
  }

  for (uint8_t i = 0; i < 64; i++) {
    uint8_t quant_value = universal_quant_table[i] * scaled_coef;

    if (quant_value < 1) {
      quant_value = 1;
    }

    quant_table[i] = quant_value;
  }


  uint64_t blocks_width  = ceil(width/8.0);
  uint64_t blocks_height = ceil(height/8.0);

  int16_t prev_Y_DC  {};
  int16_t prev_Cb_DC {};
  int16_t prev_Cr_DC {};

  vector<int16_t>  Y_DC(blocks_width * blocks_height);
  vector<int16_t> Cb_DC(blocks_width * blocks_height);
  vector<int16_t> Cr_DC(blocks_width * blocks_height);

  vector<vector<RunLengthPair>>  Y_AC(blocks_width * blocks_height);
  vector<vector<RunLengthPair>> Cb_AC(blocks_width * blocks_height);
  vector<vector<RunLengthPair>> Cr_AC(blocks_width * blocks_height);

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
  for (uint64_t block_y = 0; block_y < blocks_height; block_y++) {
    for (uint64_t block_x = 0; block_x < blocks_width; block_x++) {
      uint64_t block_index = block_y * blocks_width + block_x;

      Block<uint8_t> block_Y_raw  {};
      Block<uint8_t> block_Cb_raw {};
      Block<uint8_t> block_Cr_raw {};

      /*******************************************************\
      * Z RGB dat vytahne jeden blok pro kazdy kanal z YCbCr.
      \*******************************************************/
      for (uint8_t pixel_y = 0; pixel_y < 8; pixel_y++) {
        for (uint8_t pixel_x = 0; pixel_x < 8; pixel_x++) {

          uint64_t image_x = block_x * 8 + pixel_x;
          uint64_t image_y = block_y * 8 + pixel_y;

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


          uint64_t input_pixel_index  = image_y * width + image_x;
          uint64_t output_pixel_index = pixel_y * 8 + pixel_x;

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

      Block<int16_t> block_Y_zigzag  {};
      Block<int16_t> block_Cb_zigzag {};
      Block<int16_t> block_Cr_zigzag {};

      /*******************************************************\
      * Provede doprednou cosinovu transformaci.
      * Blok kvantizuje kvantizacni tabulkou.
      * Preskupi data do zigzag poradi.
      \*******************************************************/
      for (uint8_t v = 0; v < 8; v++) {
        for (uint8_t u = 0; u < 8; u++) {
          uint8_t pixel_index = v * 8 + u;

          double sumY  {};
          double sumCb {};
          double sumCr {};

          for (uint8_t y = 0; y < 8; y++) {
            for (uint8_t x = 0; x < 8; x++) {
              uint8_t trans_pixel_index = y * 8 + x;

              double cosc1 = cos(((2 * x + 1) * u * JPEG2D_PI ) / 16);
              double cosc2 = cos(((2 * y + 1) * v * JPEG2D_PI ) / 16);

              sumY  += (block_Y_raw[trans_pixel_index] - 128) * cosc1 * cosc2;
              sumCb += (block_Cb_raw[trans_pixel_index] - 128) * cosc1 * cosc2;
              sumCr += (block_Cr_raw[trans_pixel_index] - 128) * cosc1 * cosc2;
            }
          }

          double normU {u == 0 ? 1/sqrt(2) : 1};
          double normV {v == 0 ? 1/sqrt(2) : 1};

          double invariant_coef = 0.25 * normU * normV / quant_table[pixel_index];

          block_Y_zigzag[zigzag_index_table[pixel_index]]  = round(invariant_coef * sumY);
          block_Cb_zigzag[zigzag_index_table[pixel_index]] = round(invariant_coef * sumCb);
          block_Cr_zigzag[zigzag_index_table[pixel_index]] = round(invariant_coef * sumCr);
        }
      }

      runLengthDiffEncode(block_Y_zigzag, Y_DC[block_index], prev_Y_DC, Y_AC[block_index], weights_luma_DC, weights_luma_AC);
      runLengthDiffEncode(block_Cb_zigzag, Cb_DC[block_index], prev_Cb_DC, Cb_AC[block_index], weights_chroma_DC, weights_chroma_AC);
      runLengthDiffEncode(block_Cr_zigzag, Cr_DC[block_index], prev_Cr_DC, Cr_AC[block_index], weights_chroma_DC, weights_chroma_AC);
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

  output.write("JPEG-2D\n", 8);

  uint64_t swapped_w = toBigEndian(width);
  uint64_t swapped_h = toBigEndian(height);

  output.write(reinterpret_cast<char *>(&swapped_w), sizeof(uint64_t));
  output.write(reinterpret_cast<char *>(&swapped_h), sizeof(uint64_t));

  output.write(reinterpret_cast<char *>(quant_table.data()), quant_table.size());
  output.write(reinterpret_cast<char *>(quant_table.data()), quant_table.size());

  writeHuffmanTable(codelengths_luma_DC, output);
  writeHuffmanTable(codelengths_luma_AC, output);
  writeHuffmanTable(codelengths_chroma_DC, output);
  writeHuffmanTable(codelengths_chroma_AC, output);

  OBitstream bitstream(output);

  for (uint64_t i = 0; i < blocks_width * blocks_height; i++) {
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

void runLengthDiffEncode(const Block<int16_t> &block_zigzag, int16_t &DC, int16_t &prev_DC, vector<RunLengthPair> &AC, map<uint8_t, uint64_t> &weights_DC, map<uint8_t, uint64_t> &weights_AC) {
  uint8_t zeroes {};
  for (uint8_t pixel_index = 0; pixel_index < 64; pixel_index++) {
    if (pixel_index == 0) {
      DC = block_zigzag[pixel_index] - prev_DC;
      prev_DC = block_zigzag[pixel_index];

      weights_DC[huffmanSymbol({0, DC})]++;
    }
    else {
      if (block_zigzag[pixel_index] == 0) {
        zeroes++;
      }
      else {
        while (zeroes >= 16) {
          AC.push_back({15, 0});
          zeroes -= 16;

          weights_AC[huffmanSymbol({15, 0})]++;
        }
        RunLengthPair pair {zeroes, block_zigzag[pixel_index]};

        AC.push_back(pair);
        zeroes = 0;

        weights_AC[huffmanSymbol(pair)]++;
      }
    }
  }
  AC.push_back({0, 0});

  weights_AC[huffmanSymbol({0, 0})]++;
}

vector<pair<uint64_t, uint8_t>> huffmanGetCodelengths(const map<uint8_t, uint64_t> &weights) {
  vector<pair<uint64_t, uint8_t>> A {};

  A.reserve(weights.size());

  for (auto &pair: weights) {
    A.push_back({pair.second, pair.first});
  }

  sort(A.begin(), A.end());

  // SOURCE: http://hjemmesider.diku.dk/~jyrki/Paper/WADS95.pdf

  uint64_t n = A.size();

  uint64_t s = 1;
  uint64_t r = 1;

  for (uint64_t t = 1; t <= n - 1; t++) {
    uint64_t sum = 0;
    for (uint8_t i = 0; i < 2; i++) {
      if ((s > n) || ((r < t) && (A[r-1].first < A[s-1].first))) {
        sum += A[r-1].first;
        A[r-1].first = t;
        r++;
      }
      else {
        sum += A[s-1].first;
        s++;
      }
    }
    A[t-1].first = sum;
  }

  if (n >= 2) {
    A[n - 2].first = 0;
  }

  for (int64_t t = n - 2; t >= 1; t--) {
    A[t-1].first = A[A[t-1].first-1].first + 1;
  }

  int64_t a  = 1;
  int64_t u  = 0;
  uint64_t d = 0;
  uint64_t t = n - 1;
  uint64_t x = n;

  while (a > 0) {
    while ((t >= 1) && (A[t-1].first == d)) {
      u++;
      t--;
    }
    while (a > u) {
      A[x-1].first = d;
      x--;
      a--;
    }
    a = 2 * u;
    d++;
    u = 0;
  }

  sort(A.begin(), A.end());

  return A;
}

map<uint8_t, Codeword> huffmanGenerateCodewords(const vector<pair<uint64_t, uint8_t>> &codelengths) {
  map<uint8_t, Codeword> codewords {};

  // TODO PROVE ME

  uint8_t prefix_ones = 0;

  uint8_t codeword {};
  for (auto &pair: codelengths) {
    codewords[pair.second];

    for (uint8_t i = 0; i < prefix_ones; i++) {
      codewords[pair.second].push_back(1);
    }

    uint8_t len = pair.first - prefix_ones;

    for (uint8_t k = 0; k < len; k++) {
      codewords[pair.second].push_back(bitset<8>(codeword)[7 - k]);
    }

    codeword = ((codeword >> (8 - len)) + 1) << (8 - len);
    while (codeword > 127) {
      prefix_ones++;
      codeword <<= 1;
    }
  }

  return codewords;
}

void writeHuffmanTable(const vector<pair<uint64_t, uint8_t>> &codelengths, ofstream &stream) {
  uint8_t codelengths_cnt = codelengths.back().first + 1;
  stream.put(codelengths_cnt);

  auto it = codelengths.begin();
  for (uint8_t i = 0; i < codelengths_cnt; i++) {
    uint16_t leaves = 0;
    while ((it < codelengths.end()) && ((*it).first == i)) {
      leaves++;
      it++;
    }
    stream.put(leaves);
  }

  for (auto &pair: codelengths) {
    stream.put(pair.second);
  }
}

void encodeOnePair(const RunLengthPair &pair, const map<uint8_t, Codeword> &table, OBitstream &stream) {
  uint8_t huff_class = huffmanClass(pair.amplitude);
  uint8_t symbol     = huffmanSymbol(pair);

  stream.write(table.at(symbol));

  int16_t amplitude = pair.amplitude;
  if (amplitude < 0) {
    amplitude = -amplitude;
    amplitude = ~amplitude;
  }

  for (int8_t i = huff_class - 1; i >= 0; i--) {
    stream.writeBit((amplitude & (1 << i)) >> i);
  }
}


uint8_t huffmanClass(int16_t amplitude) {
  if (amplitude < 0) {
    amplitude = -amplitude;
  }

  uint8_t huff_class = 0;
  while (amplitude > 0) {
    amplitude = amplitude >> 1;
    huff_class++;
  }

  return huff_class;
}

uint8_t huffmanSymbol(const RunLengthPair &pair) {
  return pair.zeroes << 4 | huffmanClass(pair.amplitude);
}
