#include "bitmap.h"

Bitmap::Bitmap(unsigned int width, unsigned int height, unsigned int depth):
m_width(width),
m_height(height),
m_depth(depth),
m_pixel_width(3 * (depth > 255 ? 2 : 1)) {
  m_data = new uint8_t[m_width * m_height * m_pixel_width];
}

Bitmap::~Bitmap() {
  delete m_data;
}

uint64_t index(uint64_t x, uint64_t y) {
  return y * width() + x;
}

double red(unsigned int x, unsigned int y) {
  if (m_depth < 255) {
    return m_data[pixelWidth() * index(x, y) + 0] / double(depth());
  }
    return reinterpret_cast<uint16_t>(m_data[pixelWidth() * index(x, y) + 0]) / double(depth());
  }
  else {

  }
}
