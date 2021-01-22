/**
* @file colorspace.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Handy functions for color space conversion.
*/

#pragma once

#include <cmath>
#include <cstdint>

/**
* @brief Set of functions for conversion RGB - YCbCr.
*/
namespace YCbCr {

  /**
  * @brief  Function which converts RGB to Y.
  * @param  R Red sample.
  * @param  G Green sample.
  * @param  B Blue sample.
  * @return Y sample.
  */
  inline float RGBToY(uint16_t R, uint16_t G, uint16_t B) {
    return (0.299 * R) + (0.587 * G) + (0.114 * B);
  }

  /**
  * @brief  Function which converts RGB to Cb.
  * @param  R Red sample.
  * @param  G Green sample.
  * @param  B Blue sample.
  * @return Cb sample.
  */
  inline float RGBToCb(uint16_t R, uint16_t G, uint16_t B) {
    return - (0.168736 * R) - (0.331264 * G) + (0.5 * B);
  }

  /**
  * @brief  Function which converts RGB to Cr.
  * @param  R Red sample.
  * @param  G Green sample.
  * @param  B Blue sample.
  * @return Cr sample.
  */
  inline float RGBToCr(uint16_t R, uint16_t G, uint16_t B) {
    return (0.5 * R) - (0.418688 * G) - (0.081312 * B);
  }

  /**
  * @brief  Function which converts YCbCr to R.
  * @param  Y  Y sample.
  * @param  Cr Cr sample.
  * @return R sample.
  */
  inline float YCbCrToR(float Y, float, float Cr) {
    return Y + 1.402 * Cr;
  }

  /**
  * @brief  Function which converts YCbCr to G.
  * @param  Y  Y sample.
  * @param  Cb Cb sample.
  * @param  Cr Cr sample.
  * @return G sample.
  */
  inline float YCbCrToG(float Y, float Cb, float Cr) {
    return Y - 0.344136 * Cb - 0.714136 * Cr;
  }

  /**
  * @brief  Function which converts YCbCr to B.
  * @param  Y  Y sample.
  * @param  Cb Cb sample.
  * @return B sample.
  */
  inline float YCbCrToB(float Y, float Cb, float) {
    return Y + 1.772 * Cb;
  }
}

/**
* @brief Set of functions for conversion RGB - YCoCg.
*/
namespace YCoCg {

  /**
  * @brief  Function which converts RGB to Y.
  * @param  R Red sample.
  * @param  G Green sample.
  * @param  B Blue sample.
  * @return Y sample.
  */
  inline float RGBToY(uint16_t R, uint16_t G, uint16_t B) {
    return (0.25 * R) + (0.5 * G) + (0.25 * B);
  }

  /**
  * @brief  Function which converts RGB to Co.
  * @param  R Red sample.
  * @param  B Blue sample.
  * @return Co sample.
  */
  inline float RGBToCo(uint16_t R, uint16_t, uint16_t B) {
    return (0.5 * R) - (0.5 * B);
  }

  /**
  * @brief  Function which converts RGB to Cg.
  * @param  R Red sample.
  * @param  G Green sample.
  * @param  B Blue sample.
  * @return Cg sample.
  */
  inline float RGBToCg(uint16_t R, uint16_t G, uint16_t B) {
    return - (0.25 * R) + (0.5 * G) - (0.25 * B);
  }

  /**
  * @brief  Function which converts YCoCg to R.
  * @param  Y  Y sample.
  * @param  Co Co sample.
  * @param  Cg Cg sample.
  * @return R sample.
  */
  inline float YCoCgToR(float Y, float Co, float Cg) {
    return Y + Co - Cg;
  }

  /**
  * @brief  Function which converts YCoCg to G.
  * @param  Y  Y sample.
  * @param  Cg Cg sample.
  * @return G sample.
  */
  inline float YCoCgToG(float Y, float, float Cg) {
    return Y + Cg;
  }

  /**
  * @brief  Function which converts YCoCg to B.
  * @param  Y  Y sample.
  * @param  Co Co sample.
  * @param  Cg Cg sample.
  * @return B sample.
  */
  inline float YCoCgToB(float Y, float Co, float Cg) {
    return Y - Co - Cg;
  }
}
