/**
* @file bitstream.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Stream wrapper classes for streaming bits.
*/

#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <cstdint>

#include <istream>
#include <ostream>
#include <vector>

/**
 * @brief Base class for streaming bits. This class serves as a wrapper for standard c++ streams.
 */
class Bitstream {
public:
  /**
   * @brief The default constructor.
   */
  Bitstream(): m_index{}, m_accumulator{} {};

protected:
  uint8_t m_index;
  uint8_t m_accumulator;
};

/**
 * @brief Class for reading bits from stream.
 */
class IBitstream: public Bitstream {
public:
  /**
   * @brief The default constructor. This constructor initializes the internal stream pointer to nullptr;
   */
  IBitstream(): m_stream{} {}

  /**
   * @brief Constructor specifying a stream pointer.
   * @param stream The input stream to be read.
   */
  IBitstream(std::istream *stream): m_stream{ stream } { m_index = 8; }

  /**
   * @brief This method (re)opens bitstream to new source.
   * @param stream The input stream pointer to read.
   */
  void open(std::istream *stream) { m_stream = stream; m_index = 8; }

  /**
   * @brief Method which reads specific number of bits from stream.
   * @param size Number of bits to be read.
   * @return Vector of bits.
   */
  std::vector<bool> read(const size_t size);

  /**
   * @brief Method which reads one bit from stream.
   * @return One bit from stream.
   */
  bool readBit();

  /**
   * @brief Method which checks the stream for eof.
   * @return True if EOF, else false.
   */
  bool eof() { return m_stream->eof(); }

private:
  std::istream *m_stream;
};

/**
 * @brief Class for writing bits to stream.
 */
class OBitstream: public Bitstream {
public:
  /**
   * @brief The default constructor. This constructor initializes the internal stream pointer to nullptr;
   */
  OBitstream(): m_stream {} {}

  /**
   * @brief Constructor specifying a stream pointer.
   * @param stream The output stream pointer to write.
   */
  OBitstream(std::ostream *stream): m_stream{ stream } {}

  /**
   * @brief Destructor flushing the output buffer to stream.
   */
  ~OBitstream() { flush(); }

  /**
   * @brief This method (re)opens bitstream to new destination.
   * @param stream The output stream pointer to write.
   */
  void open(std::ostream *stream) { m_stream = stream; }

  /**
   * @brief Method which writes vector of bits to the stream.
   * @param data Vector of bits.
   */
  void write(const std::vector<bool> &data);

  /**
   * @brief Method which writes one bit to the stream.
   * @param bit Bit value..
   */
  void writeBit(const bool bit);

  /**
   * @brief Method which flushes the output buffer to file.
   */
  void flush();

private:
  std::ostream *m_stream;
};

#endif
