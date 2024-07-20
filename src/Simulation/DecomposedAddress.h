#pragma once
#include <cmath>
#include <cstdint>

struct DecomposedAddress {
    std::uint32_t tag;
    std::uint32_t index;
    std::uint32_t offset;
};

constexpr inline std::uint32_t safeCeilLog2(std::uint32_t val) noexcept {
    assert(val != 0);
    auto highestSetBit = 32 - __builtin_clz(val) - 1;
    if ((1ull << highestSetBit) == val)
        return highestSetBit;
    return highestSetBit + 1;
}

constexpr inline std::uint32_t generateBitmaskForLowestNBits(std::uint32_t bits) noexcept { return (1ull << bits) - 1; }