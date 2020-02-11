/**
* @file ppm.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Tiny library for reading and writing ppm files.
*/

#ifndef PPM_H
#define PPM_H

#include <array>

#include <cstdio>
#include <cstdint>

#include "ppm_endian.h"

class PPM {
  bool        m_opened;

  uint64_t    m_width;       /**< @brief Image width in pixels.*/
  uint64_t    m_height;      /**< @brief Image height in pixels.*/
  uint32_t    m_color_depth; /**< @brief Maximum RGB value of an image.*/

  void       *m_file;
  size_t      m_header_offset;

  int parseHeader(FILE *ppm);

public:
  PPM()                       = default;
  PPM(const PPM &)            = delete;
  PPM &operator=(const PPM &) = delete;

  PPM(PPM &&);
  ~PPM();

  int createPPM(const char *file_name, uint64_t width, uint64_t height, uint32_t color_depth);

  int mmapPPM(const char *file_name);

  void *data() const {
    return static_cast<uint8_t *>(m_file) + m_header_offset;
  }

  std::array<uint16_t, 3> get(size_t index) const {
    uint16_t R {};
    uint16_t G {};
    uint16_t B {};

    if (m_color_depth > 255) {
      const BigEndian<uint16_t> *ptr = static_cast<const BigEndian<uint16_t> *>(data());
      R = ptr[index * 3 + 0];
      G = ptr[index * 3 + 1];
      B = ptr[index * 3 + 2];
    }
    else {
      const BigEndian<uint8_t> *ptr = static_cast<const BigEndian<uint8_t> *>(data());
      R = ptr[index * 3 + 0];
      G = ptr[index * 3 + 1];
      B = ptr[index * 3 + 2];
    }

    return {R, G, B};
  }

  void put(size_t index, const std::array<uint16_t, 3> &value) {
    if (m_color_depth > 255) {
      BigEndian<uint16_t> *ptr = static_cast<BigEndian<uint16_t> *>(data());
      ptr[index * 3 + 0] = value[0];
      ptr[index * 3 + 1] = value[1];
      ptr[index * 3 + 2] = value[2];
    }
    else {
      BigEndian<uint8_t> *ptr = static_cast<BigEndian<uint8_t> *>(data());
      ptr[index * 3 + 0] = value[0];
      ptr[index * 3 + 1] = value[1];
      ptr[index * 3 + 2] = value[2];
    }
  }

  uint64_t width() const {
    return m_width;
  }

  uint64_t height() const {
    return m_height;
  }

  uint32_t color_depth() const {
    return m_color_depth;
  }
};

/**
 * @brief Base structure for reading and writing PPM files.
 */
struct PPMFileStruct {
  FILE    *file;        /**< @brief A file pointer for image reading and writing.*/
  uint64_t width;       /**< @brief Image width in pixels.*/
  uint64_t height;      /**< @brief Image height in pixels.*/
  uint32_t color_depth; /**< @brief Maximum RGB value of an image.*/
};

/**
 * @brief Structure containing one pixel worth of values.
 */
struct Pixel {
  uint16_t r; /**< @brief Red   channel.*/
  uint16_t g; /**< @brief Green channel.*/
  uint16_t b; /**< @brief Blue  channel.*/
};

/**
 * @brief Function which allocates memory for one row of pixels of specified width.
 * @param width Image width.
 * @return Pointer to the allocated memory.
 */
Pixel *allocPPMRow(size_t width);

/**
 * @brief Function which frees memory for one row of pixels.
 * @param Pointer to the allocated memory.
 */
void freePPMRow(Pixel *row);

/**
 * @brief Function which reads header of a ppm file.
 * @param ppm Pointer to the ppm base structure.
 * @return Nonzero when something went wrong, zero else.
 */
int readPPMHeader(PPMFileStruct *ppm);

/**
 * @brief Function which reads one row from file.
 * @param ppm Pointer to the ppm base structure.
 * @param buffer Pointer to the allocated buffer.
 * @return Nonzero when something went wrong, zero else.
 */
int readPPMRow(PPMFileStruct *ppm, Pixel *buffer);

/**
 * @brief Function which writes header of a ppm file.
 * @param ppm Pointer to the ppm base structure.
 * @return Nonzero when something went wrong, zero else.
 */
int writePPMHeader(const PPMFileStruct *ppm);

/**
 * @brief Function which writes one row to file.
 * @param ppm Pointer to the ppm base structure.
 * @param buffer Pointer to the filled buffer.
 * @return Nonzero when something went wrong, zero else.
 */
int writePPMRow(PPMFileStruct *ppm, const Pixel *buffer);

int mapPPM(char *file_name, uint64_t &width, uint64_t &height, uint32_t &color_depth, void *data);

#endif
