#ifndef IMAGE_JPEG_H
#define IMAGE_JPEG_H

class ImageJPEG {
public:
  ImageJPEG() {}
  ~ImageJPEG() {}

private:
  uint16_t m_width;
  uint16_t m_height;
  uint8_t *m_data;
};

#endif
