#pragma once

#include <cstdint>

struct InternalRequestCPURead {
    std::uint32_t cachelineNr;
    std::uint32_t offsetInBytes;
};

struct InternalRequestCPUWrite {
    std::uint32_t cachelineNr;
    std::uint32_t offsetInBytes;
    std::uint32_t data;
};

struct InternalRequestMemoryRead {
    std::uint32_t cachelineNr;
    std::uint8_t* const data;
    // no offset / size because entire cache line gets read
};

struct InternalRequestMemoryWrite {
    std::uint32_t cachelineNr;
};
