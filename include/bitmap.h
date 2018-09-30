#ifndef BITMAP_H
#define BITMAP_H

#include <cstdint>

class Bitmap {
public:
  Bitmap(unsigned int width, unsigned int height, unsigned int depth);
  ~Bitmap();

  double red(unsigned int x, unsigned int y);
  double green(unsigned int x, unsigned int y);
  double blue(unsigned int x, unsigned int y);

  double Y(unsigned int x, unsigned int y);
  double Cb(unsigned int x, unsigned int y);
  double Cr(unsigned int x, unsigned int y);


  uint64_t width() { return m_width; }
  uint64_t height() { return m_height; }
  uint32_t depth() { return m_depth; }

  uint8_t pixelWidth() { return m_pixel_width; }

  uint64_t size() { return m_width * m_height * m_pixel_width; }
  uint8_t *data() { return m_data; }

private:
  const uint64_t m_width;
  const uint64_t m_height;
  const uint32_t m_depth;
  const uint8_t m_pixel_width;
  uint8_t *m_data;
};

#endif
