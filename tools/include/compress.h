/**
* @file compress.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Common helper function for compression tools.
*/

#ifndef COMPRESS_H
#define COMPRESS_H

#include <cstdint>

/**
 * @brief Fucntion which prints usage of a compress tool.
 * @param argv0 Tool name.
 */
void print_usage(const char *argv0);

/**
 * @brief Fucntion which parses command line arguments.
 * @param argc Argument count.
 * @param argv Pointer to argument array.
 * @param input_file_mask Return value for a tool which contains input file mask.
 * @param output_file_name Return value for a tool which containts output file name.
 * @param quality Return value containing quality coefficient.
 */
bool parse_args(int argc, char *argv[], const char *&input_file_mask, const char *&output_file_name, float &quality);

#endif
