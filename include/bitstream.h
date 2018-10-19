#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdint.h>

#include <fstream>
#include <vector>

using namespace std;

class Bitstream {
public:
  Bitstream();
  ~Bitstream();

protected:
  int8_t m_index;
  int8_t m_accumulator;
};

class IBitstream: public Bitstream {
public:
  IBitstream(ifstream &stream);
  ~IBitstream();

  vector<bool> read(const uint64_t size);
  bool readBit();
  bool eof();

private:
  ifstream &m_stream;
};

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
