#include "jpeg2d.h"

JPEG2D::JPEG2D():
  width(0),
  height(0),
  quant_table_luma(),
  quant_table_chroma(),
  m_huffman_table_luma_DC(),
  m_huffman_table_luma_AC(),
  m_huffman_table_chroma_AC(),
  m_huffman_table_chroma_DC() {}

JPEG2D::~JPEG2D() {}
