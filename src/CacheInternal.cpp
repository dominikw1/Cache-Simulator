#include "CacheInternal.h"

#include <assert.h>
#include <bitset>
#include <iostream>

// Debug purposes
static void printLine(const std::vector<std::uint8_t>& line) {
    for (const auto& el : line) {
        std::cout << std::bitset<8>{el}.to_string() << " ";
    }
    std::cout << std::endl;
}

CacheInternal::CacheInternal(const std::uint32_t cachelineNr, const std::uint32_t cachelineSize)
    : cachelineSize{cachelineSize},
      cache{std::vector<std::vector<std::uint8_t>>(cachelineNr, std::vector<std::uint8_t>(cachelineSize, 0))} {}

void CacheInternal::handleRAMRead(const InternalRequestMemoryRead query) {
    auto& cacheline = cache[query.cachelineNr];
    cacheline.insert(cacheline.end(), &query.data[0], &query.data[cachelineSize]);
}

std::pair<std::vector<std::uint8_t>::iterator, std::vector<std::uint8_t>::iterator>
CacheInternal::handleRAMWrite(const InternalRequestMemoryWrite query) {
    return {cache[query.cachelineNr].begin(), cache[query.cachelineNr].end()};
}

std::uint32_t CacheInternal::handleCPURead(const InternalRequestCPURead query) {
    assert(query.offsetInBytes + 3 < cachelineSize);
    auto& cacheline = cache[query.cachelineNr];
    return (cacheline[query.offsetInBytes + 0] << 0) + (cacheline[query.offsetInBytes + 1] << 8) +
           (cacheline[query.offsetInBytes + 2] << 16) + (cacheline[query.offsetInBytes + 3] << 24);
}

void CacheInternal::handleCPUWrite(const InternalRequestCPUWrite query) {
    auto& cacheline = cache[query.cachelineNr];
    cacheline[query.offsetInBytes + 0] = (query.data >> 0) & ((1 << 8) - 1);
    cacheline[query.offsetInBytes + 1] = (query.data >> 8) & ((1 << 8) - 1);
    cacheline[query.offsetInBytes + 2] = (query.data >> 16) & ((1 << 8) - 1);
    cacheline[query.offsetInBytes + 3] = (query.data >> 24) & ((1 << 8) - 1);
}
