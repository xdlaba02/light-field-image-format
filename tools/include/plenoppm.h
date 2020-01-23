/**
* @file plenoppm.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Module for reading and writin ppm plenoptic images.
*/

#ifndef PLENOPPM_H
#define PLENOPPM_H

#include <ppm.h>

#include <cstdint>
#include <vector>

int mapPPMs(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, std::vector<PPM> &data);
void unmapPPMs(std::vector<PPM> &data);

/**
 * @brief Function which reads file headers from mask and checks if they are the same.
 * @param input_file_mask The mask of files to be checked.
 * @param width Returned images width.
 * @param height Returned images height.
 * @param color_depth Returned images color depth.
 * @param image_count Returned image count.
 * @return True if all headers are the same, false if not.
 */
bool checkPPMheaders(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, uint64_t &image_count);

/**
 * @brief Function which reads the ppm files into allocated memory.
 * @param input_file_mask The mask of files to be read.
 * @param rgb_data Pointer to the memory allocated by client.
 * @return True if all no problem, false if problem.
 */
bool loadPPMs(const char *input_file_mask, void *rgb_data);

/**
 * @brief Function which saves the ppm files.
 * @param output_file_mask The mask of files to be written.
 * @param rgb_data Pointer to the memory filled by client.
 * @param width Images width.
 * @param height Images height.
 * @param color_depth Images color depth.
 * @param image_count Image count.
 * @return True if all no problem, false if problem.
 */
bool savePPMs(const char *output_file_mask, const void *rgb_data, uint64_t width, uint64_t height, uint32_t color_depth, uint64_t image_count);

#endif
