#include "SubRequest.h"


//TODO: document why this works
std::vector<SubRequest> splitRequestIntoSubRequests(Request request, std::uint32_t cacheLineSizeInByte) {
    std::uint32_t currAlignedAddr = (request.addr / cacheLineSizeInByte) * cacheLineSizeInByte;
    std::uint8_t remainingBits = 32;
    std::vector<SubRequest> subrequests;
    while (currAlignedAddr < request.addr + 4) {
        std::uint32_t startPoint = std::max(request.addr, currAlignedAddr);
        std::uint8_t size = std::min(request.addr + 4, currAlignedAddr + cacheLineSizeInByte) - startPoint;
        std::uint32_t data = (request.data >> (32 - remainingBits)) & ((1ull << size * 8) - 1ull);
        std::uint8_t bitsBefore = 32 - remainingBits;

        remainingBits -= size * 8;
        currAlignedAddr += cacheLineSizeInByte;
        subrequests.push_back(SubRequest{startPoint, size, data, bitsBefore, request.we});
    }
    return subrequests;
}

std::uint32_t applyPartialRead(SubRequest subReq, std::uint32_t curr, std::uint32_t newVal) {
    return curr | (newVal << subReq.bitsBefore);
}