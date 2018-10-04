#ifndef BITSTREAM_H
#define BITSTREAM_H

class Bitstream {
public:
  Bitstream(): m_bit_index(0), m_buffer(0), m_data() {}

  void append(uint16_t data, uint8_t length) {
    std::bitset<16> bits(data);
    bits <<= 16 - length;
    for (uint8_t i = 0; i < length; i++) {
      writeBit(bits[i]);
    }
  }

  void flush() {
    m_data.push_back(m_buffer);
    m_buffer = 0;
    m_bit_index = 0;
  }

  std::vector<uint8_t> &data() {
    return m_data;
  }

private:
  void writeBit(bool bit) {
    m_buffer |= bit << (7 - m_bit_index);
    m_bit_index++;
    if (m_bit_index >= 8) {
      flush();
    }
  }

  uint8_t m_bit_index;
  uint8_t m_buffer;
  std::vector<uint8_t> m_data;
};

#endif
