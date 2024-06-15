#pragma once
#include <cstdint>
#include <vector>

#include "InternalRequests.h"

class CacheInternal {
  private:
    // maybe make std::unique_ptr<T[]> for less overhead?
    const std::uint32_t cachelineSize;
    std::vector<std::vector<std::uint8_t>> cache;

  public:
    CacheInternal(const std::uint32_t cachelineNr,
                  const std::uint32_t cachelineSize);
    void handleRAMRead(const InternalRequestMemoryRead query);
    std::pair<std::vector<std::uint8_t>::iterator,
              std::vector<std::uint8_t>::iterator>
    handleRAMWrite(const InternalRequestMemoryWrite query);
    std::uint32_t handleCPURead(const InternalRequestCPURead query);
    void handleCPUWrite(const InternalRequestCPUWrite query);
};