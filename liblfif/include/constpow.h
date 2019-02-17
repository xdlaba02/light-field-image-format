/******************************************************************************\
* SOUBOR: constpow.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef CONSTPOW_H
#define CONSTPOW_H

template<class T>
inline constexpr T constpow(const T base, const unsigned exponent) {
    return (exponent == 0) ? 1 : (base * constpow(base, exponent - 1));
}

#endif
