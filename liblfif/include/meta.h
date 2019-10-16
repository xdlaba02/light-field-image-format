/**
* @file meta.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Compile-time stuff.
*/

#ifndef META_H
#define META_H

/**
* @brief  Function which performs integer exponentiation.
* @param  base Base of exponentiation.
* @param  exponent Exponent.
* @return base to exponent
*/
template<class T>
inline constexpr T constpow(const T base, const unsigned exponent) {
    return (exponent == 0) ? 1 : (base * constpow(base, exponent - 1));
}

/*
template<size_t D, typename F>
void meta_for(size_t range[D], F &&callback) {
  for (size_t i { 0 }; i < range[D - 1]; i++) {
    auto new_callback = [&](size_t indices[D - 1]) {
      size_t new_indices[D] =
      callback()
    }
  }
}
*/

#endif
