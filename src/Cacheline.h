#pragma once
#include <cstdint>
#include <ostream>
#include <vector>

struct Cacheline {
    bool isUsed = false; // let's pretend this is a single bit
    std::uint32_t tag = 0;
    std::vector<std::uint8_t> data;
};

// Debug purposes
static inline std::ostream& operator<<(std::ostream& os, const Cacheline& cacheline) {
    os << "Cacheline used: " << cacheline.isUsed << "\n";
    os << "Tag: " << cacheline.tag << "\n";
    os << "Data: \n";
    for (const auto& byte : cacheline.data) {
        os << unsigned(byte) << " ";
    }
    os << "\n";
    return os;
}