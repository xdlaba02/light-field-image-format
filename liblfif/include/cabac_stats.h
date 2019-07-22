/**
* @file cabac_stats.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 22. 7. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Module for measuring symbol statistics.
*/

#ifndef CABAC_STAT_H
#define CABAC_STAT_H

#include <map>

template<size_t BS, size_t D>
struct CabacStats {
  std::map<bool, size_t>                    coded_block_flag_stat;
  Block<std::map<bool, size_t>, BS, D>      last_significant_coef_flag_stat;
  Block<std::map<QDATAUNIT, size_t>, BS, D> coef_level_stat;
};

#endif
