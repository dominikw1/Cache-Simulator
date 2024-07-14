#pragma once

#include <cstdint>
#include <vector>

#include "Request.h"

struct SubRequest {
    std::uint32_t addr;
    std::uint8_t size;
    std::uint32_t data;
    std::uint8_t bitsBefore;
    int we;
};

std::vector<SubRequest> splitRequestIntoSubRequests(Request request, std::uint32_t cacheLineSizeInByte);
std::uint32_t applyPartialRead(SubRequest subReq, std::uint32_t curr, std::uint32_t newVal);