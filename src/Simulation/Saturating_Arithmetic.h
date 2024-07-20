#pragma once
#include <limits>

// Try to emulate C++26 add_sat etc.

template <typename T> constexpr inline T addSatUnsigned(T a, T b) noexcept {
    T c;
    if (!__builtin_add_overflow(a, b, &c))
        return c;
    return std::numeric_limits<T>::max();
}

template <typename T> constexpr inline T mulSatUnsigned(T a, T b) noexcept {
    T c;
    if (!__builtin_mul_overflow(a, b, &c))
        return c;
    return std::numeric_limits<T>::max();
}

template <typename T> constexpr inline T powSatUnsigned(T a, T b) noexcept {
    if (a == 0)
        return 0;
    T c = 1;
    for (std::size_t i = 0; i < b; ++i) {
        c = mulSatUnsigned(a, c);
    }
    return c;
}
