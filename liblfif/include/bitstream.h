/******************************************************************************\
* SOUBOR: bitstream.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <cstdint>

#include <istream>
#include <ostream>
#include <vector>

/******************************************************************************\
* TODO komentar
\******************************************************************************/
class Bitstream {
public:
  Bitstream(): m_index{}, m_accumulator{} {};

protected:
  uint8_t m_index;
  uint8_t m_accumulator;
};

/******************************************************************************\
* TODO komentar
\******************************************************************************/
class IBitstream: public Bitstream {
public:
  IBitstream() = default;
  IBitstream(std::istream *stream): m_stream{ stream } {}

  void open(std::istream *stream) { m_stream = stream; }

  std::vector<bool> read(const size_t size);
  bool readBit();
  bool eof() { return m_stream->eof(); }

private:
  std::istream *m_stream;
};

/******************************************************************************\
* TODO komentar
\******************************************************************************/
class OBitstream: public Bitstream {
public:
  OBitstream() = default;
  OBitstream(std::ostream *stream): m_stream{ stream } {}
  ~OBitstream() { flush(); }

  void open(std::ostream *stream) { m_stream = stream; }

  void write(const std::vector<bool> &data);
  void writeBit(const bool bit);
  void flush();

private:
  std::ostream *m_stream;
};

#endif
