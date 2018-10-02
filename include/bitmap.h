#ifndef BITMAP_H
#define BITMAP_H

#include <cstdint>

template <unsigned int N>
class Bitmap {
public:
  Bitmap(): m_width(0), m_height(0), m_data(nullptr) {}

  Bitmap(uint64_t width, uint64_t height): m_width(width), m_height(height), m_data(nullptr) {
    m_data = new uint8_t[width * height * N];
  }

  ~Bitmap() {
    delete m_data;
  }

  void init(uint64_t width, uint64_t height) {
    m_width = width;
    m_height = height;
    delete m_data;
    m_data = new uint8_t[width * height * N];
  }

  bool initialized() const { return m_data != nullptr; }

  uint64_t width() const { return m_width; }
  uint64_t height() const { return m_height; }

  uint64_t sizeInPixels() const { return m_width * m_height; }
  uint64_t sizeInBytes() const { return m_width * m_height * N; }

  uint8_t *data() const { return m_data; }

private:
  uint64_t m_width;
  uint64_t m_height;
  uint8_t *m_data;
};

using BitmapRGB = Bitmap<3>;
using BitmapG = Bitmap<1>;

#endif
