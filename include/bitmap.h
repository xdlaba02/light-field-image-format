#ifndef BITMAP_H
#define BITMAP_H

#include <cstdint>

struct PixelRGB {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
}

using Pixel

template <class T>
class Bitmap {
public:
  Bitmap():
    m_width(0),
    m_height(0),
    m_image_data(0) {}

  Bitmap(uint64_t width, uint64_t height):
    m_width(width),
    m_height(height),
    m_image_data(width * height, 0) {}

  ~Bitmap() { }

  void init(uint64_t width, uint64_t height) {
    m_width = width;
    m_height = height;
    m_image_data.resize(width * height);
  }

  T *at(uint64_t x, uint64_t y) const {
    if ((x > m_width) || (y > m_height)) {
      return nullptr;
    }
    else {
      return m_image_data.at(y * m_width + x);
    }
  }

  uint64_t width() const { return m_width; }
  uint64_t height() const { return m_height; }

private:
  uint64_t m_width;
  uint64_t m_height;
  std::vector<T> m_image_data;
};

using BitmapRGB = Bitmap<3>;
using BitmapG = Bitmap<1>;

#endif
