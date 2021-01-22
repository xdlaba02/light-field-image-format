/**
* @file decompress.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Common helper function for decompression tools.
*/

#pragma once

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
 * @param input_file_name Return value for a tool which contains input file name.
 * @param output_file_mask Return value for a tool which containts output file mask.
 */
bool parse_args(int argc, char *argv[], const char *&input_file_name, const char *&output_file_mask);
