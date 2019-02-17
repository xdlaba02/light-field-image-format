/******************************************************************************\
* SOUBOR: bitstream.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <cstdint>

#include <iosfwd>
#include <vector>

/******************************************************************************\
* TODO komentar
\******************************************************************************/
class Bitstream {
public:
  Bitstream();
  ~Bitstream();

protected:
  int8_t m_index;
  int8_t m_accumulator;
};

/******************************************************************************\
* TODO komentar
\******************************************************************************/
class IBitstream: public Bitstream {
public:
  IBitstream(std::ifstream &stream);
  ~IBitstream();

  std::vector<bool> read(const size_t size);
  bool readBit();
  bool eof();

private:
  std::ifstream &m_stream;
};

/******************************************************************************\
* TODO komentar
\******************************************************************************/
class OBitstream: public Bitstream {
public:
  OBitstream(std::ofstream &stream);
  ~OBitstream();

  void write(const std::vector<bool> &data);
  void writeBit(const bool bit);
  void flush();

private:
  std::ofstream &m_stream;
};

#endif
