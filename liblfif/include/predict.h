/**
* @file predict.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 9. 8. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Prediction stuff.
*/

#ifndef PREDICT_H
#define PREDICT_H

#include "lfiftypes.h"

#include <cstdint>
#include <cassert>

template<size_t BS, size_t D, typename T>
void getSlice(const Block<T, BS, D> &block, Block<T, BS, D - 1> &slice, size_t direction, size_t index) {
  assert(direction < D);
  assert(index < BS);

  for (size_t i { 0 }; i < constpow(BS, D - 1); i++) {
    size_t directional_index = i % constpow(BS, direction) + i / constpow(BS, direction) * constpow(BS, direction + 1);
    slice[i] = block[directional_index + index * constpow(BS, direction)];
  }
}

template<size_t BS, size_t D, typename T>
void putSlice(Block<T, BS, D> &block, const Block<T, BS, D - 1> &slice, size_t direction, size_t index) {
  assert(direction < D);
  assert(index < BS);

  for (size_t i { 0 }; i < constpow(BS, D - 1); i++) {
    size_t directional_index = i % constpow(BS, direction) + i / constpow(BS, direction) * constpow(BS, direction + 1);
    block[directional_index + index * constpow(BS, direction)] = slice[i];
  }
}

template<size_t BS, size_t D>
void predict_perpendicular(Block<INPUTUNIT, BS, D> &predicted, const size_t block_dims[D], const INPUTUNIT *decoded, size_t direction) {
  assert(direction < D);

  auto multDims = [&](size_t n) {
    size_t product { 1 };
    for (size_t i { 0 }; i < n; i++) {
      product *= block_dims[i] * BS;
    }
    return product;
  };

  for (size_t j { 0 }; j < BS; j++) {
    for (size_t i { 0 }; i < constpow(BS, D - 1); i++) {
      const INPUTUNIT *ptr { decoded };

      size_t pow_val { 0 };
      for (size_t d {0}; d < D; d++) {
        if (d == direction) {
          ptr -= multDims(d);
        }
        else {
          ptr += multDims(d) * ((i % constpow(BS, pow_val + 1)) / constpow(BS, pow_val));
          pow_val++;
        }
      }

      size_t dst_index = i % constpow(BS, direction) + i / constpow(BS, direction) * constpow(BS, direction + 1);
      predicted[dst_index + j * constpow(BS, direction)] = *ptr;
    }
  }
}

template<size_t BS, size_t D>
void predict_DC(Block<INPUTUNIT, BS, D> &predicted, const size_t block_dims[D], const INPUTUNIT *decoded) {
  INPUTUNIT avg { 0 };

  auto multDims = [&](size_t n) {
    size_t product { 1 };
    for (size_t i { 0 }; i < n; i++) {
      product *= block_dims[i] * BS;
    }
    return product;
  };

  for (size_t dir { 0 }; dir < D; dir++) {
    for (size_t i { 0 }; i < constpow(BS, D - 1); i++) {
      const INPUTUNIT *ptr { decoded };

      size_t pow_val { 0 };
      for (size_t d {0}; d < D; d++) {
        if (d == dir) {
          ptr -= multDims(d);
        }
        else {
          ptr += multDims(d) * ((i % constpow(BS, pow_val + 1)) / constpow(BS, pow_val));
          pow_val++;
        }
      }

      avg += *ptr;
    }
  }

  avg /= constpow(BS, D - 1) * D;

  for (size_t i { 0 }; i < constpow(BS, D); i++) {
    predicted[i] = avg;
  }
}

template<size_t BS, size_t D>
void predict_diagonal(Block<INPUTUNIT, BS, D> &predicted, const size_t block_dims[D], const INPUTUNIT *decoded) {

  auto multDims = [&](size_t n) {
    size_t product { 1 };
    for (size_t i { 0 }; i < n; i++) {
      product *= block_dims[i] * BS;
    }
    return product;
  };

  for (size_t i = 0; i < constpow(BS, D); i++) {
    size_t shortest {};
    const INPUTUNIT *ptr { decoded };

    for (size_t d = 0; d < D; d++) {
      ptr += (i % constpow(BS, d + 1)) / constpow(BS, d) * multDims(d);
    }

    shortest = (i % constpow(BS, 1)) / constpow(BS, 0);
    for (size_t d = 1; d < D; d++) {
      size_t next = (i % constpow(BS, d + 1)) / constpow(BS, d);

      if (next < shortest) {
        shortest = next;
      }
    }

    for (size_t d = 0; d < D; d++) {
      ptr -= multDims(d) * (shortest + 1);
    }

    predicted[i] = *ptr;
  }
}

template <size_t BS, size_t D>
void predict_direction(Block<INPUTUNIT, BS, D> &output, const int8_t direction[D], const INPUTUNIT *src, const size_t input_stride[D + 1]) {
  Block<INPUTUNIT, BS * 2 + 1, D - 1> ref {};

  int64_t ptr_offset { 0 };
  int64_t ref_offset { 0 };
  int64_t project_offset { 0 };
  size_t  main_ref   { 0 };

  // find which neighbouring block will be main
  for (size_t d = 0; d < D; d++) {
    if (direction[d] >= direction[main_ref]) {
      main_ref = d;
    }
  }

  if (direction[main_ref] <= 0) {
    return;
  }

  // find offset for main neighbour to make space for projected samples
  size_t idx {};
  for (size_t d { 0 }; d < D; d++) {
    if (d != main_ref) {
      if (direction[d] >= 0) {
        ref_offset     += constpow(BS * 2 + 1, idx) * BS;
        project_offset += constpow(BS * 4 + 2, d) * BS;
        idx++;
      }
    }
  }

  // move pointer to the start of reference samples instead of start of predicted block
  for (size_t d { 0 }; d < D; d++) {
    ptr_offset -= input_stride[d];
  }

  // project neighbour samples to main reference
  for (size_t d { 0 }; d < D; d++) {
    if (d != main_ref) {
      if (direction[d] > 0) {
        for (size_t i { 0 }; i < constpow(BS * 2 + 1, D - 1); i++) {
          int64_t src_idx {};
          int64_t dst_idx {};

          std::array<int64_t, D> dst_pos {};

          for (size_t dd = 0; dd < D; dd++) {
            src_idx += (i % constpow(BS * 2 + 1, dd + 1)) / constpow(BS * 2 + 1, dd) * input_stride[dd];
          }

          src_idx = src_idx % input_stride[d] + src_idx / input_stride[d] * input_stride[d + 1];

          //scale index from (BS * 2 + 1) ^ D to (BS * 4 + 2) ^ D while retaining position
          for (size_t dd = 0; dd < D; dd++) {
            dst_idx += (i % constpow(BS * 2 + 1, dd + 1)) / constpow(BS * 2 + 1, dd) * constpow(BS * 4 + 2, dd);
          }

          //rotate index to direction d
          dst_idx = dst_idx % constpow(BS * 4 + 2, d) + dst_idx / constpow(BS * 4 + 2, d) * constpow(BS * 4 + 2, d + 1);

          dst_idx += project_offset;

          bool overflow {};
          for (size_t dd { 0 }; dd < D; dd++) {
            dst_pos[dd] = dst_idx % constpow(BS * 4 + 2, dd + 1) / constpow(BS * 4 + 2, dd) * direction[main_ref];

            if (dst_pos[dd] >= static_cast<int64_t>((BS * 2 + 1) * direction[main_ref])) {
              overflow = true;
            }
          }

          if (overflow) {
            continue;
          }

          while (dst_pos[main_ref] > 0) {
            for (size_t dd { 0 }; dd < D; dd++) {
              dst_pos[dd] -= direction[dd];
            }
          }

          for (size_t dd { 0 }; dd < D; dd++) {
            if (dst_pos[dd] < 0) {
              overflow = true;
            }
            else if (dst_pos[dd] >= static_cast<int64_t>((BS * 2 + 1) * direction[main_ref])) {
              overflow = true;
            }
          }

          if (overflow) {
            continue;
          }

          dst_idx = 0;
          size_t pow {};
          for (size_t dd { 0 }; dd < D; dd++) {
            if (dd != main_ref) {
              dst_idx += dst_pos[dd] / direction[main_ref] * constpow(BS * 2 + 1, pow);
              pow++;
            }
          }
          // Tady by se taky mozna hodilo kontrolovat, jestli tam uz neni nejaka hodnota s mensi chybou
          ref[dst_idx] = src[src_idx + ptr_offset];
        }
      }
    }
  }

  // copy main samples to main reference
  for (size_t i { 0 }; i < constpow(BS * 2 + 1, D - 1); i++) {
    int64_t src_idx {};
    size_t dst_idx {};

    for (size_t dd = 0; dd < D; dd++) {
      src_idx += (i % constpow(BS * 2 + 1, dd + 1)) / constpow(BS * 2 + 1, dd) * input_stride[dd];
    }

    src_idx = src_idx % input_stride[main_ref] + src_idx / input_stride[main_ref] * input_stride[main_ref + 1];

    dst_idx = i + ref_offset;

    bool overflow {};
    for (size_t dd = 0; dd < D; dd++) {
      if (((dst_idx % constpow(BS * 2 + 1, dd + 1)) / constpow(BS * 2 + 1, dd)) < ((ref_offset % constpow(BS * 2 + 1, dd + 1)) / constpow(BS * 2 + 1, dd))) {
        overflow = true;
      }
    }

    if (overflow) {
      continue;
    }

    ref[dst_idx] = src[src_idx + ptr_offset];
  }

  for (size_t i { 0 }; i < constpow(BS, D); i++) {
    std::array<int64_t, D> pos {};

    for (size_t d { 0 }; d < D; d++) {
      pos[d] = ((i % constpow(BS, d + 1) / constpow(BS, d)) + 1) * direction[main_ref];
    }

    while (pos[main_ref] > 0) {
      for (size_t d { 0 }; d < D; d++) {
        pos[d] -= direction[d];
      }
    }


    int64_t dst_idx {};
    size_t pow {};
    for (size_t d { 0 }; d < D; d++) {
      if (d != main_ref) {
        // hodnoty by se mohly interpolovat?
        dst_idx += pos[d] / direction[main_ref] * constpow(BS * 2 + 1, pow);
        pow++;
      }
    }
    output[i] = ref[dst_idx + ref_offset];
  }
}

#if 0

void predict_angle_HEVC(int bitDepth, const Pel* pSrc, Int srcStride, Pel* pTrueDst, Int dstStrideTrue, UInt uiWidth, UInt uiHeight, const Bool bEnableEdgeFilters) {
   Int width  = Int(uiWidth);
   Int height = Int(uiHeight);

   const Bool       bIsModeVer         = (dirMode >= 18);
   const Int        intraPredAngleMode = (bIsModeVer) ? (Int)dirMode - VER_IDX :  -((Int)dirMode - HOR_IDX);
   const Int        absAngMode         = abs(intraPredAngleMode);
   const Int        signAng            = intraPredAngleMode < 0 ? -1 : 1;
   const Bool       edgeFilter         = bEnableEdgeFilters && isLuma(channelType) && (width <= MAXIMUM_INTRA_FILTERED_WIDTH) && (height <= MAXIMUM_INTRA_FILTERED_HEIGHT);

   // Set bitshifts and scale the angle parameter to block size
   static const Int angTable[9]    = {0,    2,    5,   9,  13,  17,  21,  26,  32};
   static const Int invAngTable[9] = {0, 4096, 1638, 910, 630, 482, 390, 315, 256}; // (256 * 32) / Angle
   Int invAngle                    = invAngTable[absAngMode];
   Int absAng                      = angTable[absAngMode];
   Int intraPredAngle              = signAng * absAng;

   Pel* refMain;
   Pel* refSide;

   Pel  refAbove[2 * MAX_CU_SIZE + 1];
   Pel   refLeft[2 * MAX_CU_SIZE + 1];

   // Initialize the Main and Left reference array.
   if (intraPredAngle < 0) {
     const Int refMainOffsetPreScale = (bIsModeVer ? height : width ) - 1;
     const Int refMainOffset         = height - 1;

     for (Int x = 0; x < width + 1; x++) {
       refAbove[x + refMainOffset] = pSrc[x - srcStride - 1];
     }

     for (Int y = 0; y < height + 1; y++) {
       refLeft[y + refMainOffset] = pSrc[(y - 1) * srcStride - 1];
     }

     refMain = (bIsModeVer ? refAbove : refLeft)  + refMainOffset;
     refSide = (bIsModeVer ? refLeft  : refAbove) + refMainOffset;

     // Extend the Main reference to the left.
     Int invAngleSum = 128;       // rounding for (shift by 8)
     for (Int k = -1; k > (refMainOffsetPreScale + 1) * intraPredAngle >> 5; k--) {
       invAngleSum += invAngle;
       refMain[k] = refSide[invAngleSum >> 8];
     }
   }
   else {
     for (Int x = 0; x < 2 * width + 1; x++) {
       refAbove[x] = pSrc[x - srcStride - 1];
     }

     for (Int y = 0; y < 2 * height + 1; y++) {
       refLeft[y] = pSrc[(y - 1) * srcStride - 1];
     }

     refMain = bIsModeVer ? refAbove : refLeft ;
     refSide = bIsModeVer ? refLeft  : refAbove;
   }

   // swap width/height if we are doing a horizontal mode:
   Pel tempArray[MAX_CU_SIZE * MAX_CU_SIZE];
   const Int dstStride = bIsModeVer ? dstStrideTrue : MAX_CU_SIZE;
   Pel *pDst = bIsModeVer ? pTrueDst : tempArray;
   if (!bIsModeVer) {
     std::swap(width, height);
   }

   // pure vertical or pure horizontal
   if (intraPredAngle == 0) {
     for (Int y = 0; y < height; y++) {
       for (Int x = 0; x < width; x++) {
         pDst[y * dstStride + x] = refMain[x + 1];
       }
     }

     if (edgeFilter) {
       for (Int y = 0; y < height; y++) {
         pDst[y * dstStride] = Clip3(0, ((1 << bitDepth) - 1), pDst[y * dstStride] + ((refSide[y + 1] - refSide[0]) >> 1));
       }
     }
   }
   else {
     Pel *pDsty = pDst;

     for (Int y = 0, deltaPos = intraPredAngle; y < height; y++, deltaPos += intraPredAngle, pDsty += dstStride) {
       const Int deltaInt   = deltaPos >> 5;
       const Int deltaFract = deltaPos & (32 - 1);

       if (deltaFract) {
         // Do linear filtering
         const Pel *pRM = refMain + deltaInt + 1;
         Int lastRefMainPel =* pRM++;
         for (Int x = 0; x < width; pRM++, x++) {
           Int thisRefMainPel =* pRM;
           pDsty[x + 0] = (Pel)(((32 - deltaFract) * lastRefMainPel + deltaFract * thisRefMainPel + 16) >> 5);
           lastRefMainPel = thisRefMainPel;
         }
       }
       else {
         // Just copy the integer samples
         for (Int x = 0; x < width; x++) {
           pDsty[x] = refMain[x + deltaInt + 1];
         }
       }
     }
   }

   // Flip the block if this is the horizontal mode
   if (!bIsModeVer) {
     for (Int y = 0; y < height; y++) {
       for (Int x = 0; x < width; x++) {
         pTrueDst[x * dstStrideTrue] = pDst[x];
       }

       pTrueDst++;
       pDst += dstStride;
     }
   }
 }

#endif

#endif
