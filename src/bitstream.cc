#include "bitstream.h"

#include <iostream>

using namespace std;

Bitstream::Bitstream(): m_index{}, m_accumulator{} {}
Bitstream::~Bitstream() {}

IBitstream::IBitstream(ifstream &stream): m_stream{stream} {}
IBitstream::~IBitstream() {}

OBitstream::OBitstream(ofstream &stream): m_stream{stream} {}
OBitstream::~OBitstream() {}

vector<bool> IBitstream::read(const uint64_t size) {
  vector<bool> data {};
  for (uint64_t i = 0; i < size; i++) {
    data.push_back(readBit());
  }
  return data;
}

bool IBitstream::readBit() {
  if (m_index <= 0) {
    m_stream.read(reinterpret_cast<char *>(&m_accumulator), sizeof(char));
    m_index = 8;
  }

  bool data {(m_accumulator & (1 << (m_index - 1))) >> (m_index - 1)};

  m_index--;

  return data;
}

bool IBitstream::eof() {
  return m_stream.eof();
}

void OBitstream::write(const vector<bool> &data) {
  for (auto &&bit: data) {
    writeBit(bit);
  }
}

void OBitstream::writeBit(const bool bit) {
  if (m_index > 7) {
    flush();
  }

  m_accumulator |= bit << (7 - m_index);
  m_index++;
}

void OBitstream::flush() {
  if (m_index > 0) {
    m_stream.write(reinterpret_cast<char *>(&m_accumulator), sizeof(m_accumulator));
    m_index = 0;
    m_accumulator = 0;
  }
}
