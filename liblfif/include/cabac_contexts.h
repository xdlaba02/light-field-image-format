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
struct CABACContexts {
  CABACContexts(size_t amp_bits): DC_ctxs(5 * 4 + 2 * amp_bits - 1), AC_ctxs(3 * (constpow(BS, D) - 1) + 4 * (amp_bits - 1)) {}

  std::vector<CABAC::ContextModel> DC_ctxs;
  std::vector<CABAC::ContextModel> AC_ctxs;
};

#endif
