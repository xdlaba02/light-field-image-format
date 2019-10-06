/**
* @file ppm.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Tiny library for reading and writing ppm files.
*/

#ifndef PPM_H
#define PPM_H

#include <cstdio>
#include <cstdint>

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

#endif
