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
  static const size_t NUM_GREATER_ONE_CTXS { 5 };
  static const size_t NUM_ABS_LEVEL_CTXS   { 5 };

  CABAC::ContextModel               coded_block_flag_ctx;
  Block<CABAC::ContextModel, BS, D> significant_coef_flag_ctx;
  Block<CABAC::ContextModel, BS, D> last_significant_coef_flag_ctx;
  CABAC::ContextModel               coef_greater_one_ctx[NUM_GREATER_ONE_CTXS];
  CABAC::ContextModel               coef_abs_level_ctx[NUM_ABS_LEVEL_CTXS];
};

#endif
