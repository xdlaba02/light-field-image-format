/******************************************************************************\
* SOUBOR: file_mask.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef FILE_MASK_H
#define FILE_MASK_H

#include <string>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

class FileMask {
public:
  FileMask(const string &input_file_mask);

  string operator [](size_t index) const;

  size_t count();

private:
  string m_filename_mask;
  vector<size_t> m_mask_indexes;
};

#endif
