#include "file_mask.h"

#include <cmath>
#include <iomanip>

FileMask::FileMask(const string &input_file_mask): m_filename_mask{input_file_mask}, m_mask_indexes{} {
  for (size_t i = 0; m_filename_mask[i] != '\0'; i++) {
    if (m_filename_mask[i] == '#') {
      m_mask_indexes.push_back(i);
    }
  }
}

string FileMask::operator [](size_t index) const {
  string file_name {m_filename_mask};

  stringstream image_number {};
  image_number << setw(m_mask_indexes.size()) << setfill('0') << to_string(index);

  for (size_t i = 0; i < m_mask_indexes.size(); i++) {
    file_name[m_mask_indexes[i]] = image_number.str()[i];
  }

  return file_name;
}

size_t FileMask::count() {
  return pow(10, m_mask_indexes.size());
}
