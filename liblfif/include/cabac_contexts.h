/**
* @file cabac_contexts.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 16. 7. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Context-Adaptive Binary Arithmetic Coding.
*/

#ifndef CABAC_CONTEXTS_H
#define CABAC_CONTEXTS_H

template <size_t D>
struct CABACContextsH264 {
  static const size_t NUM_GREATER_ONE_CTXS { 10 };
  static const size_t NUM_ABS_LEVEL_CTXS   { 10 };

  CABAC::ContextModel                  coded_block_flag_ctx;
  DynamicBlock<CABAC::ContextModel, D> significant_coef_flag_ctx;
  DynamicBlock<CABAC::ContextModel, D> last_significant_coef_flag_ctx;
  CABAC::ContextModel                  coef_greater_one_ctx[NUM_GREATER_ONE_CTXS];
  CABAC::ContextModel                  coef_abs_level_ctx[NUM_ABS_LEVEL_CTXS];

  CABACContextsH264(const std::array<size_t, D> &BS):
    coded_block_flag_ctx{},
    significant_coef_flag_ctx(BS),
    last_significant_coef_flag_ctx(BS) {}
};

template <size_t D>
struct CABACContextsJPEG {
  CABACContextsJPEG(const std::array<size_t, D> &BS, size_t amp_bits): DC_ctxs(5 * 4 + 2 * amp_bits - 1), AC_ctxs(3 * (get_stride<D>(BS) - 1) + 4 * (amp_bits - 1)) {}

  std::vector<CABAC::ContextModel> DC_ctxs;
  std::vector<CABAC::ContextModel> AC_ctxs;
};

template <size_t D>
struct CABACContextsDIAGONAL {
  std::vector<CABAC::ContextModel> coded_diag_flag_ctx;
  std::vector<CABAC::ContextModel> last_coded_diag_flag_ctx;
  std::vector<CABAC::ContextModel> significant_coef_flag_high_ctx;
  std::vector<CABAC::ContextModel> significant_coef_flag_low_ctx;
  std::vector<CABAC::ContextModel> coef_greater_one_ctx;
  std::vector<CABAC::ContextModel> coef_greater_two_ctx;
  std::vector<CABAC::ContextModel> coef_abs_level_ctx;

  CABACContextsDIAGONAL(const std::array<size_t, D> &BS):
      coded_diag_flag_ctx(num_diagonals<D>(BS)),
      last_coded_diag_flag_ctx(num_diagonals<D>(BS)),
      significant_coef_flag_high_ctx(D + 1),
      significant_coef_flag_low_ctx(D + 1),
      coef_greater_one_ctx(num_diagonals<D>(BS)),
      coef_greater_two_ctx(num_diagonals<D>(BS)),
      coef_abs_level_ctx(num_diagonals<D>(BS))
      {}
};

template <class T>
inline constexpr T pow(const T base, const uint64_t exponent) {
    return (exponent == 0) ? 1 : (base * pow(base, exponent - 1));
}


template <size_t D>
struct CABACContextsPredictionMode {
  std::array<CABAC::ContextModel, pow(5, D) + 3> prediction_ctx;
};

template<size_t D>
struct CABACContextsRUNLENGTH {
  std::array<CABAC::ContextModel, constpow(2, 16)> coef_DC_ctx;
  std::array<CABAC::ContextModel, constpow(2, 16)> coef_AC_ctx;
};

#endif
