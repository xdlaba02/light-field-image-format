/*******************************************************\
* SOUBOR: jpeg2d.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#include "jpeg2d.h"
#include "endian.h"
#include "jpeg.h"
#include "jpeg_encoder.h"
#include "jpeg_decoder.h"

#include <cstring>
#include <ctime>

#include <iostream>
#include <iomanip>

bool RGBtoJPEG2D(const char *output_filename, const vector<uint8_t> &rgb_data, const uint64_t w, const uint64_t h, const uint64_t ix, const uint64_t iy, const uint8_t quality) {
  clock_t clock_start {};
  cerr << fixed << setprecision(3);

  cerr << "CONVERTING TO YCbCr" << endl;
  clock_start = clock();

  vector<uint8_t> Y_data  = convertRGB(rgb_data, RGBtoY);
  vector<uint8_t> Cb_data = convertRGB(rgb_data, RGBtoCb);
  vector<uint8_t> Cr_data = convertRGB(rgb_data, RGBtoCr);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "CONSTRUCTING QUANTIZATION TABLE" << endl;
  clock_start = clock();

  QuantTable<2> quant_table = constructQuantTable<2>(quality);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "CONSTRUCTING ZIGZAG TABLE" << endl;
  clock_start = clock();

  ZigzagTable<2> zigzag_table = constructZigzagTable<2>(quant_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "COVERTING TO BLOCKS" << endl;
  clock_start = clock();

  vector<Block<uint8_t, 2>> blocks_Y  = convertToBlocks<2>(Y_data,  {w, h*ix*iy});
  vector<Block<uint8_t, 2>> blocks_Cb = convertToBlocks<2>(Cb_data, {w, h*ix*iy});
  vector<Block<uint8_t, 2>> blocks_Cr = convertToBlocks<2>(Cr_data, {w, h*ix*iy});

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "SHIFTING VALUES TO <-128, 127>" << endl;
  clock_start = clock();

  vector<Block<int8_t, 2>> blocks_Y_shifted  = shiftBlocks<2>(blocks_Y);
  vector<Block<int8_t, 2>> blocks_Cb_shifted = shiftBlocks<2>(blocks_Cb);
  vector<Block<int8_t, 2>> blocks_Cr_shifted = shiftBlocks<2>(blocks_Cr);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "FORWARD DISCRETE COSINE TRANSFORMING" << endl;
  clock_start = clock();

  vector<Block<float, 2>> blocks_Y_transformed  = transformBlocks<2>(blocks_Y_shifted);
  vector<Block<float, 2>> blocks_Cb_transformed = transformBlocks<2>(blocks_Cb_shifted);
  vector<Block<float, 2>> blocks_Cr_transformed = transformBlocks<2>(blocks_Cr_shifted);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "QUANTIZING" << endl;
  clock_start = clock();

  vector<Block<int16_t, 2>> blocks_Y_quantized  = quantizeBlocks<2>(blocks_Y_transformed, quant_table);
  vector<Block<int16_t, 2>> blocks_Cb_quantized = quantizeBlocks<2>(blocks_Cb_transformed, quant_table);
  vector<Block<int16_t, 2>> blocks_Cr_quantized = quantizeBlocks<2>(blocks_Cr_transformed, quant_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "ZIGZAGING" << endl;
  clock_start = clock();

  vector<Block<int16_t, 2>> blocks_Y_zigzag  = zigzagBlocks<2>(blocks_Y_quantized,  zigzag_table);
  vector<Block<int16_t, 2>> blocks_Cb_zigzag = zigzagBlocks<2>(blocks_Cb_quantized, zigzag_table);
  vector<Block<int16_t, 2>> blocks_Cr_zigzag = zigzagBlocks<2>(blocks_Cr_quantized, zigzag_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "RUNLENGTH AND DIFF ENCODING" << endl;
  clock_start = clock();

  vector<vector<RunLengthPair>> runlenght_Y  = runLenghtDiffEncodeBlocks<2>(blocks_Y_zigzag);
  vector<vector<RunLengthPair>> runlenght_Cb = runLenghtDiffEncodeBlocks<2>(blocks_Cb_zigzag);
  vector<vector<RunLengthPair>> runlenght_Cr = runLenghtDiffEncodeBlocks<2>(blocks_Cr_zigzag);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "COUNTING RUNLENGTH PAIR WEIGHTS" << endl;
  clock_start = clock();

  map<uint8_t, uint64_t> weights_luma_AC   {};
  map<uint8_t, uint64_t> weights_luma_DC   {};

  map<uint8_t, uint64_t> weights_chroma_AC {};
  map<uint8_t, uint64_t> weights_chroma_DC {};

  huffmanGetWeights(runlenght_Y,  weights_luma_AC, weights_luma_DC);
  huffmanGetWeights(runlenght_Cb, weights_chroma_AC, weights_chroma_DC);
  huffmanGetWeights(runlenght_Cr, weights_chroma_AC, weights_chroma_DC);

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

  output.write("JPEG-2D\n", 8);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "WRITING IMAGE DIMENSIONS" << endl;
  clock_start = clock();

  uint64_t raw_w = toBigEndian(w);
  uint64_t raw_h = toBigEndian(h);
  uint64_t raw_ix = toBigEndian(ix);
  uint64_t raw_iy = toBigEndian(iy);

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
  cerr << "HUFFMAN ENCODING AND WRITING BLOCKS" << endl;
  clock_start = clock();

  OBitstream bitstream(output);

  encodePairs(runlenght_Y,  huffcodes_luma_AC,   huffcodes_luma_DC,   bitstream);
  encodePairs(runlenght_Cb, huffcodes_chroma_AC, huffcodes_chroma_DC, bitstream);
  encodePairs(runlenght_Cr, huffcodes_chroma_AC, huffcodes_chroma_DC, bitstream);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "FLUSHING OUTPUT" << endl;
  clock_start = clock();

  bitstream.flush();

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  return true;
}





bool JPEG2DtoRGB(const char *input_filename, uint64_t &w, uint64_t &h, uint64_t &ix, uint64_t &iy, vector<uint8_t> &rgb_data) {
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
  if (strncmp(magic_number, "JPEG-2D\n", 8) != 0) {
    return false;
  }

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "READING IMAGE DIMENSIONS" << endl;
  clock_start = clock();

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
  uint64_t height = h * iy * ix;

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "READING QUANTIZATION TABLES" << endl;
  clock_start = clock();

  QuantTable<2> quant_table {};
  input.read(reinterpret_cast<char *>(quant_table.data()), quant_table.size());

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "CONSTRUCTING ZIGZAG TABLE" << endl;
  clock_start = clock();

  ZigzagTable<2> zigzag_table = constructZigzagTable<2>(quant_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "READING HUFFMAN TABLES" << endl;
  clock_start = clock();

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

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "READING AND HUFFMAN DECODING BLOCKS" << endl;
  clock_start = clock();

  uint64_t blocks_width  = ceil(width/8.0);
  uint64_t blocks_height = ceil(height/8.0);

  uint64_t blocks_cnt = blocks_width*blocks_height;

  IBitstream bitstream(input);

  vector<vector<RunLengthPair>> runlenght_Y  = decodePairs(huff_counts_luma_DC,   huff_counts_luma_AC,   huff_symbols_luma_DC,   huff_symbols_luma_AC,   blocks_cnt, bitstream);
  vector<vector<RunLengthPair>> runlenght_Cb = decodePairs(huff_counts_chroma_DC, huff_counts_chroma_AC, huff_symbols_chroma_DC, huff_symbols_chroma_AC, blocks_cnt, bitstream);
  vector<vector<RunLengthPair>> runlenght_Cr = decodePairs(huff_counts_chroma_DC, huff_counts_chroma_AC, huff_symbols_chroma_DC, huff_symbols_chroma_AC, blocks_cnt, bitstream);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "RUNLENGTH AND DIFF DECODING" << endl;
  clock_start = clock();

  vector<Block<int16_t, 2>> blocks_Y_zigzag  = runLenghtDiffDecodePairs<2>(runlenght_Y);
  vector<Block<int16_t, 2>> blocks_Cb_zigzag = runLenghtDiffDecodePairs<2>(runlenght_Cb);
  vector<Block<int16_t, 2>> blocks_Cr_zigzag = runLenghtDiffDecodePairs<2>(runlenght_Cr);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "DEZIGZAGING" << endl;
  clock_start = clock();

  vector<Block<int16_t, 2>> blocks_Y_quantized  = dezigzagBlocks<2>(blocks_Y_zigzag,  zigzag_table);
  vector<Block<int16_t, 2>> blocks_Cb_quantized = dezigzagBlocks<2>(blocks_Cb_zigzag, zigzag_table);
  vector<Block<int16_t, 2>> blocks_Cr_quantized = dezigzagBlocks<2>(blocks_Cr_zigzag, zigzag_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "DEQUANTIZING" << endl;
  clock_start = clock();

  vector<Block<int32_t, 2>> blocks_Y_transformed  = dequantizeBlocks<2>(blocks_Y_quantized,  quant_table);
  vector<Block<int32_t, 2>> blocks_Cb_transformed = dequantizeBlocks<2>(blocks_Cb_quantized, quant_table);
  vector<Block<int32_t, 2>> blocks_Cr_transformed = dequantizeBlocks<2>(blocks_Cr_quantized, quant_table);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "INVERSE DISCRETE COSINE TRANSFORMING" << endl;
  clock_start = clock();

  vector<Block<float, 2>> blocks_Y_shifted  = detransformBlocks<2>(blocks_Y_transformed);
  vector<Block<float, 2>> blocks_Cb_shifted = detransformBlocks<2>(blocks_Cb_transformed);
  vector<Block<float, 2>> blocks_Cr_shifted = detransformBlocks<2>(blocks_Cr_transformed);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "DESHIFTING VALUES TO <0, 255>" << endl;
  clock_start = clock();

  vector<Block<uint8_t, 2>> blocks_Y  = deshiftBlocks<2>(blocks_Y_shifted);
  vector<Block<uint8_t, 2>> blocks_Cb = deshiftBlocks<2>(blocks_Cb_shifted);
  vector<Block<uint8_t, 2>> blocks_Cr = deshiftBlocks<2>(blocks_Cr_shifted);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "DEBLOCKING" << endl;
  clock_start = clock();

  vector<uint8_t> Y_data  = convertFromBlocks<2>(blocks_Y,  {w, h*ix*iy});
  vector<uint8_t> Cb_data = convertFromBlocks<2>(blocks_Cb, {w, h*ix*iy});
  vector<uint8_t> Cr_data = convertFromBlocks<2>(blocks_Cr, {w, h*ix*iy});

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;
  cerr << "COVERTING TO RGB" << endl;
  clock_start = clock();

  rgb_data = YCbCrToRGB(Y_data, Cb_data, Cr_data);

  cerr << static_cast<float>(clock() - clock_start)/CLOCKS_PER_SEC << " s" << endl;

  return true;
}
