#ifndef CONSTMATH_H
#define CONSTMATH_H

template<class T>
inline constexpr T constpow(const T base, const unsigned exponent) {
    return (exponent == 0) ? 1 : (base * constpow(base, exponent - 1));
}

#endif
