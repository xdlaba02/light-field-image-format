/******************************************************************************\
* SOUBOR: cabac_test.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <cabac.h>

#include <fstream>
#include <iostream>

using namespace std;

int main(void) {
  ofstream outputFile  {};
  OBitstream output    {};
  CABACEncoder encoder {};

  ifstream inputFile    {};
  IBitstream input     {};
  CABACDecoder decoder {};

  uint64_t input_value  { 65535 };
  uint64_t output_value {};

  outputFile.open("/tmp/cabac_test.tmp");
  output.open(&outputFile);

  encoder.init(output);
  for (size_t i = 0; i < sizeof(input_value) * 8; i++) {
    bool bit = (input_value >> i) & 1;
    encoder.encodeBitBypass(bit);
  }
  encoder.terminate();

  output.flush();
  outputFile.flush();

  inputFile.open("/tmp/cabac_test.tmp");
  input.open(&inputFile);

  decoder.init(input);
  for (size_t i = 0; i < sizeof(input_value) * 8; i++) {
    bool bit = decoder.decodeBitBypass();
    output_value |= bit << i;
  }

  cerr << output_value << endl;
}
