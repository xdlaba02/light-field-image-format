/**
* @file constpow.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Compile-time exponentiation.
*/

#ifndef CONSTPOW_H
#define CONSTPOW_H

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

#endif
