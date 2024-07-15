#pragma once
#include <cmath>
#include <cstdint>

struct DecomposedAddress {
    std::uint32_t tag;
    std::uint32_t index;
    std::uint32_t offset;
};

constexpr inline std::uint32_t safeCeilLog2(std::uint32_t val) noexcept {
    // taken from: https://stackoverflow.com/a/35758355
    return (val > 1) ? 1 + std::log2l(val >> 1) : 0;
}

constexpr inline std::uint32_t generateBitmaskForLowestNBits(std::uint8_t bits) noexcept { return (1ull << bits) - 1; }