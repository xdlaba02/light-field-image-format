/**
* @file cabac_contexts.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 16. 7. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Context-Adaptive Binary Arithmetic Coding.
*/

#ifndef CABAC_CONTEXTS_H
#define CABAC_CONTEXTS_H

template <size_t BS, size_t D>
struct CABACContextsH264 {
  static const size_t NUM_GREATER_ONE_CTXS { 10 };
  static const size_t NUM_ABS_LEVEL_CTXS   { 10 };

  CABAC::ContextModel               coded_block_flag_ctx;
  Block<CABAC::ContextModel, BS, D> significant_coef_flag_ctx;
  Block<CABAC::ContextModel, BS, D> last_significant_coef_flag_ctx;
  CABAC::ContextModel               coef_greater_one_ctx[NUM_GREATER_ONE_CTXS];
  CABAC::ContextModel               coef_abs_level_ctx[NUM_ABS_LEVEL_CTXS];
};

template <size_t BS, size_t D>
struct CABACContextsJPEG {
  CABACContextsJPEG(size_t amp_bits): DC_ctxs(5 * 4 + 2 * amp_bits - 1), AC_ctxs(3 * (constpow(BS, D) - 1) + 4 * (amp_bits - 1)) {}

  std::vector<CABAC::ContextModel> DC_ctxs;
  std::vector<CABAC::ContextModel> AC_ctxs;
};

template <size_t BS, size_t D>
struct CABACContextsDIAGONAL {
  std::array<CABAC::ContextModel, D * (BS - 1) + 1> coded_diag_flag_ctx;
  std::array<CABAC::ContextModel, D * (BS - 1) + 1> last_coded_diag_flag_ctx;
  std::array<CABAC::ContextModel, D + 1>            significant_coef_flag_high_ctx;
  std::array<CABAC::ContextModel, D + 1>            significant_coef_flag_low_ctx;
  std::array<CABAC::ContextModel, D * (BS - 1) + 1> coef_greater_one_ctx;
  std::array<CABAC::ContextModel, D * (BS - 1) + 1> coef_greater_two_ctx;
  std::array<CABAC::ContextModel, D * (BS - 1) + 1> coef_abs_level_ctx;
  std::array<CABAC::ContextModel, constpow(3, D - 1) * 4 + 4 + 1> prediction_ctx;
};

template<size_t BS, size_t D>
struct CABACContextsRUNLENGTH {
  std::array<CABAC::ContextModel, constpow(2, 16)> coef_DC_ctx;
  std::array<CABAC::ContextModel, constpow(2, 16)> coef_AC_ctx;
};

#endif
