/******************************************************************************\
* SOUBOR: file_mask.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef FILE_MASK_H
#define FILE_MASK_H

#include <vector>
#include <string>

class FileMask {
public:
  FileMask(const std::string &input_file_mask);

  std::string operator [](size_t index) const;

  size_t count();

private:
  std::string m_filename_mask;
  std::vector<size_t> m_mask_indexes;
};

#endif
