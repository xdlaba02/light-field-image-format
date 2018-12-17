/*******************************************************\
* SOUBOR: bitstream.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdint.h>

#include <fstream>
#include <vector>

using namespace std;

/*******************************************************\
* TODO komentar
\*******************************************************/
class Bitstream {
public:
  Bitstream();
  ~Bitstream();

protected:
  int8_t m_index;
  int8_t m_accumulator;
};

/*******************************************************\
* TODO komentar
\*******************************************************/
class IBitstream: public Bitstream {
public:
  IBitstream(ifstream &stream);
  ~IBitstream();

  vector<bool> read(const size_t size);
  bool readBit();
  bool eof();

private:
  ifstream &m_stream;
};

/*******************************************************\
* TODO komentar
\*******************************************************/
class OBitstream: public Bitstream {
public:
  OBitstream(ofstream &stream);
  ~OBitstream();

  void write(const vector<bool> &data);
  void writeBit(const bool bit);
  void flush();

private:
  ofstream &m_stream;
};

#endif
