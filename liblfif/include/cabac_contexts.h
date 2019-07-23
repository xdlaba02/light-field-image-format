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
  CABACContexts(size_t amp_bits): DC_S_ctxs(5), DC_XM_ctxs(amp_bits), AC_XM_ctxs(amp_bits) {}

  struct XMCtx {
    CABAC::ContextModel X;
    CABAC::ContextModel M;
  };

  struct DCSCtxs {
    CABAC::ContextModel S0;
    CABAC::ContextModel SS;
    CABAC::ContextModel SPN[2];
  };

  struct ACSCtx {
    CABAC::ContextModel SE;
    CABAC::ContextModel S0;
    CABAC::ContextModel SNSPX1;
  };

  std::vector<DCSCtxs> DC_S_ctxs;
  std::vector<XMCtx>   DC_XM_ctxs;

  Block<ACSCtx, BS, D> AC_S_ctxs;
  std::vector<XMCtx>   AC_XM_ctxs;
};

#endif
