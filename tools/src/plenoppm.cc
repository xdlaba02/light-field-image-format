/******************************************************************************\
* SOUBOR: plenoppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"
#include "file_mask.h"

#include <iostream>

using namespace std;

bool isSquare(uint64_t num) {
  for (size_t i = 0; i * i <= num; i++) {
    if (i * i == num) {
      return true;
    }
  }

  return false;
}

int mapPPMs(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, std::vector<PPM> &data) {
  FileMask file_name(input_file_mask);

  width       = 0;
  height      = 0;
  color_depth = 0;

  for (size_t image = 0; image < file_name.count(); image++) {
    PPM ppm {};

    if (ppm.mmapPPM(file_name[image].c_str()) < 0) {
      continue;
    }

    if (width && height && color_depth) {
      if ((ppm.width() != width) || (ppm.height() != height) || (ppm.color_depth() != color_depth)) {
        cerr << "ERROR: PPMs DIMENSIONS MISMATCH" << endl;
        return -1;
      }
    }

    width       = ppm.width();
    height      = ppm.height();
    color_depth = ppm.color_depth();

    data.push_back(std::move(ppm));
  }
  return 0;
}

int createPPMs(const char *output_file_mask, uint64_t width, uint64_t height, uint32_t color_depth, std::vector<PPM> &data) {
  FileMask file_name(output_file_mask);

  for (size_t image {}; image < data.size(); image++) {
    if (create_directory(file_name[image].c_str())) {
      return -1;
    }

    if (data[image].createPPM(file_name[image].c_str(), width, height, color_depth) < 0) {
      return -2;
    }
  }
  return 0;
}

bool checkPPMheaders(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, uint64_t &image_count) {
  PPMFileStruct ppm {};
  FileMask file_name(input_file_mask);

  for (size_t image = 0; image < file_name.count(); image++) {
    ppm.file = fopen(file_name[image].c_str(), "rb");
    if (!ppm.file) {
      continue;
    }

    image_count++;

    if (readPPMHeader(&ppm)) {
      cerr << "ERROR: BAD PPM HEADER" << endl;
      return false;
    }

    fclose(ppm.file);

    if (width && height && color_depth) {
      if ((ppm.width != width) || (ppm.height != height) || (ppm.color_depth != color_depth)) {
        cerr << "ERROR: PPMs DIMENSIONS MISMATCH" << endl;
        return false;
      }
    }

    width       = ppm.width;
    height      = ppm.height;
    color_depth = ppm.color_depth;
  }

  if (!image_count) {
    cerr << "ERROR: NO IMAGE LOADED" << endl;
    return false;
  }

  if (!isSquare(image_count)) {
    cerr << "ERROR: NOT SQUARE" << endl;
    return false;
  }

  return true;
}

bool loadPPMs(const char *input_file_mask, void *rgb_data) {
  PPMFileStruct ppm {};
  Pixel *ppm_row    {};
  FileMask file_name(input_file_mask);

  size_t real_image = 0;

  for (size_t image = 0; image < file_name.count(); image++) {
    ppm.file = fopen(file_name[image].c_str(), "rb");
    if (!ppm.file) {
      continue;
    }

    readPPMHeader(&ppm);

    if (!ppm_row) {
      ppm_row = allocPPMRow(ppm.width);
    }

    for (size_t row = 0; row < ppm.height; row++) {
      if (readPPMRow(&ppm, ppm_row)) {
        cerr << "ERROR: BAD PPM" << endl;
        fclose(ppm.file);
        return false;
      }

      if (ppm.color_depth < 256) {
        for (size_t col = 0; col < ppm.width; col++) {
          static_cast<uint8_t *>(rgb_data)[((real_image * ppm.height + row) * ppm.width + col) * 3 + 0] = ppm_row[col].r;
          static_cast<uint8_t *>(rgb_data)[((real_image * ppm.height + row) * ppm.width + col) * 3 + 1] = ppm_row[col].g;
          static_cast<uint8_t *>(rgb_data)[((real_image * ppm.height + row) * ppm.width + col) * 3 + 2] = ppm_row[col].b;
        }
      }
      else {
        for (size_t col = 0; col < ppm.width; col++) {
          static_cast<uint16_t *>(rgb_data)[((real_image * ppm.height + row) * ppm.width + col) * 3 + 0] = ppm_row[col].r;
          static_cast<uint16_t *>(rgb_data)[((real_image * ppm.height + row) * ppm.width + col) * 3 + 1] = ppm_row[col].g;
          static_cast<uint16_t *>(rgb_data)[((real_image * ppm.height + row) * ppm.width + col) * 3 + 2] = ppm_row[col].b;
        }
      }

    }

    fclose(ppm.file);

    real_image++;
  }

  freePPMRow(ppm_row);

  return true;
}

bool savePPMs(const char *output_file_mask, const void *rgb_data, uint64_t width, uint64_t height, uint32_t color_depth, uint64_t image_count) {
  PPMFileStruct ppm {};
  Pixel *ppm_row    {};

  ppm.width = width;
  ppm.height = height;
  ppm.color_depth = color_depth;

  ppm_row = allocPPMRow(width);

  FileMask file_name(output_file_mask);

  for (size_t image = 0; image < image_count; image++) {
    if (create_directory(file_name[image].c_str())) {
      cerr << "ERROR: CANNON OPEN " << file_name[image] << " FOR WRITING\n";
      return 1;
    }

    ppm.file = fopen(file_name[image].c_str(), "wb");
    if (!ppm.file) {
      cerr << "ERROR: CANNOT OPEN " << file_name[image] << "FOR WRITING" << endl;
      return false;
    }

    if (writePPMHeader(&ppm)) {
      cerr << "ERROR: CANNOT WRITE TO " << file_name[image] << endl;
      return false;
    }

    for (size_t row = 0; row < height; row++) {
      if (color_depth < 256) {
        for (size_t col = 0; col < ppm.width; col++) {
          ppm_row[col].r = static_cast<const uint8_t *>(rgb_data)[((image * ppm.height + row) * ppm.width + col) * 3 + 0];
          ppm_row[col].g = static_cast<const uint8_t *>(rgb_data)[((image * ppm.height + row) * ppm.width + col) * 3 + 1];
          ppm_row[col].b = static_cast<const uint8_t *>(rgb_data)[((image * ppm.height + row) * ppm.width + col) * 3 + 2];
        }
      }
      else {
        for (size_t col = 0; col < ppm.width; col++) {
          ppm_row[col].r = static_cast<const uint16_t *>(rgb_data)[((image * ppm.height + row) * ppm.width + col) * 3 + 0];
          ppm_row[col].g = static_cast<const uint16_t *>(rgb_data)[((image * ppm.height + row) * ppm.width + col) * 3 + 1];
          ppm_row[col].b = static_cast<const uint16_t *>(rgb_data)[((image * ppm.height + row) * ppm.width + col) * 3 + 2];
        }
      }


      if (writePPMRow(&ppm, ppm_row)) {
        cerr << "ERROR: CANNOT WRITE TO " << file_name[image] << endl;
        return false;
      }
    }

    fclose(ppm.file);
  }

  freePPMRow(ppm_row);

  return true;
}
