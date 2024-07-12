#pragma once

#include "ReplacementPolicy.h"
#include "Request.h"
#include "Result.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <systemc>

enum class MappingType { Direct, Fully_Associative };

struct Cacheline {
    bool isUsed = false; // let's pretend this is a single bit
    std::uint32_t tag = 0;
    std::vector<std::uint8_t> data;
};

struct DecomposedAddress {
    std::uint32_t tag;
    std::uint32_t index;
    std::uint32_t offset;
};

struct SubRequest {
    std::uint32_t addr;
    std::uint8_t size;
    std::uint32_t data;
    std::uint8_t bitsBefore;
    int we;
};

inline static std::vector<SubRequest> splitRequestIntoSubRequests(Request request, std::uint32_t cacheLineSizeInByte) {
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

inline static std::uint32_t applyPartialRead(SubRequest subReq, std::uint32_t curr, std::uint32_t newVal) {
    return curr | (newVal << subReq.bitsBefore);
}

static inline std::uint32_t safeCeilLog2(std::uint32_t val) noexcept {
    // taken from: https://stackoverflow.com/a/35758355
    return (val > 1) ? 1 + std::log2l(val >> 1) : 0;
}

template <MappingType mappingType> SC_MODULE(Cache) {
  public:
    // CPU -> Cache
    sc_core::sc_in<std::uint32_t> cpuAddrBus{"cpuAddrBus"};
    sc_core::sc_in<std::uint32_t> cpuDataInBus{"cpuDataInBus"};
    sc_core::sc_in<bool> cpuWeBus{"cpuWeBus"};
    sc_core::sc_in<bool> cpuValidRequest{"cpuValidRequest"};

    // Cache -> RAM
    sc_core::sc_out<std::uint32_t> memoryAddrBus{"memoryAddrBus"};
    std::vector<sc_core::sc_out<std::uint8_t>> memoryDataOutBusses;
    sc_core::sc_out<bool> memoryWeBus{"memoryWeBus"};
    sc_core::sc_out<bool> memoryValidRequestBus{"memoryValidRequestBus"};

    // RAM -> Cache
    std::vector<sc_core::sc_in<std::uint8_t>> memoryDataInBusses;
    sc_core::sc_in<bool> memoryReadyBus{"memoryReadyBus"};

    // Cache -> CPU
    sc_core::sc_out<bool> ready{"readyBus"};
    sc_core::sc_out<std::uint32_t> cpuDataOutBus{"cpuDataOutBus"};

    std::uint64_t hitCount = 0;
    std::uint64_t missCount = 0;

  private:
    std::unique_ptr<ReplacementPolicy<std::uint32_t>> replacementPolicy = nullptr;

    std::uint64_t numCacheLines = 0;
    std::uint64_t cacheLineSize = 0; // in Byte

    std::uint64_t cacheLatency = 0;

    std::vector<Cacheline> cacheInternal;

    // index and tag depend on type of cache, could also be precomputed but would require more template magic
    std::uint8_t numOffsetBitsInAddress = 0;

    // private since this is never to be called, just a convenient way to get proper systemc side definition
    SC_CTOR(Cache);

  public:
    Cache(sc_core::sc_module_name name, unsigned int numCacheLines, unsigned int cacheLineSize,
          unsigned int cacheLatency, std::unique_ptr<ReplacementPolicy<std::uint32_t>> policy)
        : sc_module{name}, memoryDataInBusses{cacheLineSize}, memoryDataOutBusses{cacheLineSize},
          replacementPolicy{std::move(policy)}, numCacheLines{numCacheLines}, cacheLineSize{cacheLineSize},
          cacheLatency{cacheLatency}, cacheInternal{numCacheLines},
          numOffsetBitsInAddress{static_cast<std::uint8_t>(safeCeilLog2(cacheLineSize))} {
        using namespace sc_core; // in scope as to not pollute global namespace
        for (auto& cacheline : cacheInternal) {
            cacheline.data = std::vector<std::uint8_t>(cacheLineSize, 0);
        }

        SC_THREAD(handleRequest);
        sensitive << cpuValidRequest.pos();
        dont_initialize();
    }

  private:
    constexpr DecomposedAddress decomposeAddress(std::uint32_t address) noexcept;

    std::vector<Cacheline>::iterator getCachelineOwnedByAddr(DecomposedAddress decomposedAddr);
    std::vector<Cacheline>::iterator writeRAMReadIntoCacheline(DecomposedAddress decomposedAddr);
    void registerUsage(std::vector<Cacheline>::iterator cacheline);
    std::vector<Cacheline>::iterator fetchIfNotPresent(std::uint32_t addr, DecomposedAddress decomposedAddr) {
        auto cacheline = getCachelineOwnedByAddr(decomposedAddr);
        if (cacheline != cacheInternal.end()) {
            ++hitCount;
            return cacheline;
        }
        std::cout << "MISS" << std::endl;
        ++missCount;
        startReadFromRAM(addr);
        std::cout << "Now waiting for RAM" << std::endl;
        wait(memoryReadyBus.posedge_event());
        std::cout << " Done waiting for RAM" << std::endl;
        return writeRAMReadIntoCacheline(decomposedAddr);
    }

    void handleRequest() {
        while (true) {
            std::cout << "Received request\n";
            auto request = Request{cpuAddrBus.read(), cpuDataInBus.read(), cpuWeBus.read()};
            std::cout << request << "\n";
            auto subRequests = splitRequestIntoSubRequests(request, cacheLineSize);
            ready.write(false);
            std::uint32_t readData = 0;
            for (auto& subRequest : subRequests) {
                std::cout << "Starting handling subrequest " << subRequest.addr << " " << subRequest.data << " "
                          << unsigned(subRequest.size) << " " << subRequest.we << " " << unsigned(subRequest.bitsBefore)
                          << std::endl;
                wait(cacheLatency, sc_core::SC_NS);
                std::cout << "Done waiting, now potentially reading into cache\n";
                auto addr = subRequest.addr;
                // split into tag - index - offset
                auto decomposedAddr = decomposeAddress(addr);

                // check if in cache - if not read from RAM
                auto cacheline = fetchIfNotPresent(addr, decomposedAddr);
                std::cout << "Done reading into cache\n";

                registerUsage(cacheline);

                std::cout << "Cacheline we read looks like: " << std::endl;
                for (auto& by : cacheline->data) {
                    std::cout << unsigned(by) << " ";
                }
                std::cout << std::endl;

                if (subRequest.we) {
                    doWrite(*cacheline, decomposedAddr, subRequest.data, subRequest.size);
                    passWriteOnToRAM(*cacheline, addr);
                } else {
                    auto tempReadData = doRead(decomposedAddr, *cacheline, subRequest.size);
                    std::cout << "Just read " << tempReadData << " from cache\n";
                    readData = applyPartialRead(subRequest, readData, tempReadData);
                }
                std::cout << "Done with subcycle" << std::endl;
            }
            if (!request.we) {
                std::cout << "Sending " << readData << " back to CPU" << std::endl;
                cpuDataOutBus.write(readData);
            }
            std::cout << "Done with cycle" << std::endl;
            ready.write(true);
            wait();
        }
    }

    // precondition: cacheline is in cache
    std::uint32_t doRead(DecomposedAddress decomposedAddr, Cacheline & cacheline, std::uint8_t numBytes) {
        std::cout << unsigned(numBytes) << " " << decomposedAddr.offset << std::endl;
        assert((numBytes + decomposedAddr.offset - 1) < cacheLineSize);

        std::uint32_t retVal = 0;
        for (std::size_t byteNr = 0; byteNr < numBytes; ++byteNr) {
            retVal += cacheline.data.at(decomposedAddr.offset + byteNr) << (byteNr * 8);
        }
        return retVal;
    }

    void doWrite(Cacheline & cacheline, DecomposedAddress decomposedAddr, std::uint32_t data, std::uint8_t numBytes) {
        std::cout << unsigned(numBytes) << " " << decomposedAddr.offset << std::endl;

        assert((numBytes + decomposedAddr.offset - 1) < cacheLineSize);
        std::cout<<"Attempting to write "<<data<<std::endl;
        for (std::size_t byteNr = 0; byteNr < numBytes; ++byteNr) {
            cacheline.data[(decomposedAddr.offset + byteNr)] = (data >> 8 * byteNr) & ((1 << 8) - 1);
        }
        std::cout << "Done writing into cacheline" << std::endl;
    }

    void passWriteOnToRAM(Cacheline & cacheline, std::uint32_t addr) {
        startWriteToRAM(cacheline, addr);
        std::cout << "Wainting for RAM write " << std::endl;
        wait(memoryReadyBus.posedge_event());
        std::cout << "Done waiting for RAM write " << std::endl;
        endWriteToRAM();
    }

    void endWriteToRAM() {
        wait(sc_core::SC_ZERO_TIME);
        memoryValidRequestBus.write(false);
    }

    // precondition: cacheline is in cache
    void startWriteToRAM(Cacheline & cacheline, std::uint32_t addr) {
        std::uint32_t alignedAddr = (addr / cacheLineSize) * cacheLineSize; // TODO: check if crosses boundaries
        memoryAddrBus.write(alignedAddr);
        memoryWeBus.write(true);
        for (std::size_t i = 0; i < cacheline.data.size(); ++i) {
            //    std::cout << "Writing to Cache->RAM data out bus " << i << " : " << unsigned(cacheline.data[i]) <<
            //    "
            //    ";
            memoryDataOutBusses.at(i).write(cacheline.data.at(i));
        }
        wait(sc_core::SC_ZERO_TIME);
        memoryValidRequestBus.write(true);
    }

    void startReadFromRAM(std::uint32_t addr) {
        std::uint32_t alignedAddr = (addr / cacheLineSize) * cacheLineSize; // TODO: check if crosses boundaries
        memoryAddrBus.write(alignedAddr);
        memoryWeBus.write(false);
        wait(sc_core::SC_ZERO_TIME);
        memoryValidRequestBus.write(true);
        for (std::size_t i = 0; i < memoryDataOutBusses.size(); ++i) {
            memoryDataOutBusses.at(i).write(0);
        }
    }
};

template <>
inline std::vector<Cacheline>::iterator
Cache<MappingType::Direct>::getCachelineOwnedByAddr(DecomposedAddress decomposedAddr) {
    auto cachelineExpectedAt = cacheInternal.begin() + decomposedAddr.index;
    if (cachelineExpectedAt->isUsed && cachelineExpectedAt->tag == decomposedAddr.tag) {
        return cachelineExpectedAt;
    } else {
        return cacheInternal.end();
    }
}

template <>
inline std::vector<Cacheline>::iterator
Cache<MappingType::Fully_Associative>::getCachelineOwnedByAddr(DecomposedAddress decomposedAddr) {
    auto cachelineFoundTag =
        std::find_if(cacheInternal.begin(), cacheInternal.end(),
                     [&decomposedAddr](Cacheline& cacheline) { return cacheline.tag == decomposedAddr.tag; });
    if (cachelineFoundTag == cacheInternal.end()) {
        return cacheInternal.end();
    } else {
        return cachelineFoundTag;
    }
}

template <>
inline std::vector<Cacheline>::iterator
Cache<MappingType::Direct>::writeRAMReadIntoCacheline(DecomposedAddress decomposedAddr) {
    std::cout << "Writing RAM Read into cacheline " << std::endl;
    auto cachelineToWriteInto = cacheInternal.begin() + decomposedAddr.index; // there is only one possible space
    assert(cachelineToWriteInto != cacheInternal.end());
    assert(memoryDataInBusses.size() == cachelineToWriteInto->data.size());
    std::transform(memoryDataInBusses.begin(), memoryDataInBusses.end(), cachelineToWriteInto->data.begin(),
                   [](sc_core::sc_in<std::uint8_t>& port) { return port.read(); });
    cachelineToWriteInto->isUsed = true;
    cachelineToWriteInto->tag = decomposedAddr.tag;
    wait(sc_core::SC_ZERO_TIME);
    memoryValidRequestBus.write(false);
    std::cout << "Done writing RAM Read into cacheline " << std::endl;
    return cachelineToWriteInto;
}

template <>
inline std::vector<Cacheline>::iterator
Cache<MappingType::Fully_Associative>::writeRAMReadIntoCacheline(DecomposedAddress decomposedAddr) {
    auto firstUnusedCacheline = std::find_if(cacheInternal.begin(), cacheInternal.end(),
                                             [](const Cacheline& cacheline) { return !cacheline.isUsed; });
    if (firstUnusedCacheline == cacheInternal.end()) {
        firstUnusedCacheline = cacheInternal.begin() + replacementPolicy->pop();
    }
    std::transform(memoryDataInBusses.begin(), memoryDataInBusses.end(), firstUnusedCacheline->data.begin(),
                   [](sc_core::sc_in<std::uint8_t>& port) { return port.read(); });
    firstUnusedCacheline->isUsed = true;
    firstUnusedCacheline->tag = decomposedAddr.tag;
    wait(sc_core::SC_ZERO_TIME);
    memoryValidRequestBus.write(false);
    return firstUnusedCacheline;
}

template <>
inline constexpr DecomposedAddress Cache<MappingType::Direct>::decomposeAddress(std::uint32_t address) noexcept {
    std::uint8_t numIndexBits = safeCeilLog2(numCacheLines);
    std::uint32_t offsetBitMask = (1ll << numOffsetBitsInAddress) - 1;
    std::uint32_t indexBitMask = (1ll << numIndexBits) - 1;
    std::uint32_t tagBitMask = (1ll << (32 - numIndexBits - numOffsetBitsInAddress)) - 1;
    assert(offsetBitMask > 0 && indexBitMask > 0 && tagBitMask > 0);
    return DecomposedAddress{((address >> numOffsetBitsInAddress) >> numIndexBits) & tagBitMask,
                             (address >> numOffsetBitsInAddress) & indexBitMask, address & offsetBitMask};
}

template <>
inline constexpr DecomposedAddress
Cache<MappingType::Fully_Associative>::decomposeAddress(std::uint32_t address) noexcept {
    std::uint32_t offsetBitMask = (1ll << numOffsetBitsInAddress) - 1;
    std::uint32_t tagBitMask = (1ll << (32 - numOffsetBitsInAddress)) - 1;
    assert(offsetBitMask > 0 && tagBitMask > 0);
    return DecomposedAddress{(address >> numOffsetBitsInAddress) & tagBitMask, 0, address & offsetBitMask};
}

template <> inline void Cache<MappingType::Direct>::registerUsage(std::vector<Cacheline>::iterator cacheline) {
    // no bookeeping needed
}

template <>
inline void Cache<MappingType::Fully_Associative>::registerUsage(std::vector<Cacheline>::iterator cacheline) {
    replacementPolicy->logUse(cacheline - cacheInternal.begin());
}
