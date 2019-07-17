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

  CABAC::ContextModel input_model {};

  ifstream inputFile   {};
  IBitstream input     {};
  CABACDecoder decoder {};

  CABAC::ContextModel output_model {};

  outputFile.open("/tmp/cabac_test.tmp");
  output.open(&outputFile);

  encoder.init(output);
  for (size_t i = 0; i < 256; i++) {
    encoder.encodeUEG0(input_model, i);
  }
  encoder.terminate();

  output.flush();
  outputFile.flush();

  inputFile.open("/tmp/cabac_test.tmp");
  input.open(&inputFile);

  decoder.init(input);
  for (size_t i = 0; i < 256; i++) {
    size_t val = decoder.decodeUEG0(output_model);
    cerr << val << endl;
    if (val != i) {
      cerr << "POTATO\n";
    }
  }

  decoder.terminate();
}
