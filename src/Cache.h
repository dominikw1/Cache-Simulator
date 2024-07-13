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

// Debug purposes
static inline std::ostream& operator<<(std::ostream& os, const Cacheline& cacheline) {
    os << "Cacheline used: " << cacheline.isUsed << "\n";
    os << "Tag: " << cacheline.tag << "\n";
    os << "Data: " << std::endl;
    for (auto& byte : cacheline.data) {
        os << unsigned(byte) << " ";
    }
    os << "\n";
    return os;
}

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

static constexpr inline std::uint32_t safeCeilLog2(std::uint32_t val) noexcept {
    // taken from: https://stackoverflow.com/a/35758355
    return (val > 1) ? 1 + std::log2l(val >> 1) : 0;
}

template <MappingType mappingType> SC_MODULE(Cache) {

  public:
    // Global Clock
    sc_core::sc_in<bool> clock{"Global Clock"};

    // CPU -> Cache
    sc_core::sc_in<std::uint32_t> cpuAddrBus{"cpuAddrBus"};
    sc_core::sc_in<std::uint32_t> cpuDataInBus{"cpuDataInBus"};
    sc_core::sc_in<bool> cpuWeBus{"cpuWeBus"};
    sc_core::sc_in<bool> cpuValidRequest{"cpuValidRequest"};

    // Cache -> RAM
    sc_core::sc_out<std::uint32_t> memoryAddrBus{"memoryAddrBus"};
    sc_core::sc_out<std::uint32_t> memoryDataOutBus{"memoryDataOutBus"};
    sc_core::sc_out<bool> memoryWeBus{"memoryWeBus"};
    sc_core::sc_out<bool> memoryValidRequestBus{"memoryValidRequestBus"};

    // RAM -> Cache
    sc_core::sc_in<sc_dt::sc_bv<128>> memoryDataInBus{"memoryDataInBus"};
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

    // yes this is dynamic memory allocation, but only happens during constructor
    std::vector<Cacheline> cacheInternal;

    // index and tag depend on type of cache, could also be precomputed but would require more template magic
    std::uint8_t numOffsetBitsInAddress = 0;

    // private since this is never to be called, just a convenient way to get proper systemc side definition
    SC_CTOR(Cache);

  public:
    Cache(sc_core::sc_module_name name, unsigned int numCacheLines, unsigned int cacheLineSize,
          unsigned int cacheLatency, std::unique_ptr<ReplacementPolicy<std::uint32_t>> policy)
        : sc_module{name}, replacementPolicy{std::move(policy)}, numCacheLines{numCacheLines},
          cacheLineSize{cacheLineSize}, cacheLatency{cacheLatency}, cacheInternal{numCacheLines},
          numOffsetBitsInAddress{static_cast<std::uint8_t>(safeCeilLog2(cacheLineSize))} {
        using namespace sc_core; // in scope as to not pollute global namespace
        for (auto& cacheline : cacheInternal) {
            cacheline.data = std::vector<std::uint8_t>(cacheLineSize, 0);
        }

        SC_THREAD(handleRequest);
        sensitive << clock.pos();
        dont_initialize();
    }

  private:
    constexpr DecomposedAddress decomposeAddress(std::uint32_t address) noexcept;
    std::vector<Cacheline>::iterator getCachelineOwnedByAddr(DecomposedAddress decomposedAddr);
    std::vector<Cacheline>::iterator chooseWhichCachelineToFillFromRAM(DecomposedAddress decomposedAddr);
    void registerUsage(std::vector<Cacheline>::iterator cacheline);
    void waitForRAM();

    std::vector<Cacheline>::iterator writeRAMReadIntoCacheline(DecomposedAddress decomposedAddr) {
        // he is ready after all, dont want to confuse him
        memoryValidRequestBus.write(false);

        std::cout << "Writing RAM Read into cacheline " << std::endl;
        auto cachelineToWriteInto = chooseWhichCachelineToFillFromRAM(decomposedAddr);

        // we do not allow any inputs violating this rule
        assert(cachelineToWriteInto->data.size() % 16 == 0);
        sc_dt::sc_bv<128> dataRead;
        std::uint32_t numReadEvents = (cachelineToWriteInto->data.size() / 16);

        // dont need to wait before first one because we can only get here if RAM tells us it is ready
        for (int i = 0; i < numReadEvents; ++i) {
            dataRead = memoryDataInBus.read();

            // 128 bits -> 16 bytes
            for (int byte = 0; byte < 16; ++byte) {
                cachelineToWriteInto->data[16 * i + byte] = dataRead.range(16 * i + 15, 16 * i).to_uint();
            }

            // if this is the last one we don't need to wait anymore
            if (i + 1 <= numReadEvents)
                wait(clock.posedge_event());
        }

        cachelineToWriteInto->isUsed = true;
        cachelineToWriteInto->tag = decomposedAddr.tag;

        std::cout << "Done writing RAM Read into cacheline " << std::endl;
        return cachelineToWriteInto;
    }

    std::vector<Cacheline>::iterator fetchIfNotPresent(std::uint32_t addr, DecomposedAddress decomposedAddr) {
        auto cacheline = getCachelineOwnedByAddr(decomposedAddr);
        if (cacheline != cacheInternal.end()) {
            std::cout << "HIT" << std::endl;
            ++hitCount;
            return cacheline;
        }
        std::cout << "MISS" << std::endl;
        ++missCount;
        startReadFromRAM(addr);
        waitForRAM();
        return writeRAMReadIntoCacheline(decomposedAddr);
    }

    std::uint32_t cyclesPassedInRequest = 0;

    Request constructRequestFromBusses() { return Request{cpuAddrBus.read(), cpuDataInBus.read(), cpuWeBus.read()}; }

    void handleSubRequest(SubRequest subRequest, std::uint32_t & readData) { // readData is an OUT parameter
        std::cout << "Starting handling subrequest " << subRequest.addr << " " << subRequest.data << " "
                  << unsigned(subRequest.size) << " " << subRequest.we << " " << unsigned(subRequest.bitsBefore)
                  << std::endl;

        std::cout << "Potentially reading into cache if not present...\n";
        auto addr = subRequest.addr;

        // split into tag - index - offset
        auto decomposedAddr = decomposeAddress(addr);
        // check if in cache - if not read from RAM

        auto cacheline = fetchIfNotPresent(addr, decomposedAddr);
        std::cout << "Done reading into cache\n";

        // if a fully associative Cache and we use a stateful policy we declare the use of the cacheline here
        registerUsage(cacheline);
        std::cout << "Cacheline we read looks like: " << *cacheline << std::endl;

        if (subRequest.we) {
            doWrite(*cacheline, decomposedAddr, subRequest.data, subRequest.size);
            passWriteOnToRAM(*cacheline, decomposedAddr, addr);
            std::cout << "After writing it now looks like: " << *cacheline << std::endl;
        } else {
            auto tempReadData = doRead(decomposedAddr, *cacheline, subRequest.size);
            std::cout << "Just read " << tempReadData << " from cache\n";
            readData = applyPartialRead(subRequest, readData, tempReadData);
        }

        // wait out the rest here!!
        while (cyclesPassedInRequest < cacheLatency) {
            wait(clock.posedge_event());
            ++cyclesPassedInRequest;
        }
        std::cout << "Done with subcycle" << std::endl;
    }

    void handleRequest() {
        while (true) {
            wait();
            if (!cpuValidRequest.read())
                continue;

            cyclesPassedInRequest = 0;
            // Tell CPU we are not ready yet (since he waits for us in the next cycle (not this) this is not a race)
            ready.write(false);

            auto request = constructRequestFromBusses();
            std::cout << "Received request:\n" << request << "\n";
            auto subRequests = splitRequestIntoSubRequests(request, cacheLineSize);

            std::uint32_t readData = 0;
            for (auto&& subRequest : subRequests) {
                handleSubRequest(std::move(subRequest), readData);
            }
            if (!request.we) {
                std::cout << "Sending " << readData << " back to CPU" << std::endl;
                cpuDataOutBus.write(readData);
            }
            std::cout << "Done with cycle" << std::endl;
            ready.write(true);
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
        std::cout << "Attempting to write " << data << std::endl;
        for (std::size_t byteNr = 0; byteNr < numBytes; ++byteNr) {
            cacheline.data[(decomposedAddr.offset + byteNr)] = (data >> 8 * byteNr) & ((1 << 8) - 1);
        }
        std::cout << "Done writing into cacheline" << std::endl;
    }

    void passWriteOnToRAM(Cacheline & cacheline, DecomposedAddress decomposedAddr, std::uint32_t addr) {
        std::uint32_t data = 0;

        std::size_t startByte = std::min(static_cast<std::size_t>(decomposedAddr.offset), cacheline.data.size() - 4);

        data |= (cacheline.data[startByte + 0]) << 0 * 8;
        data |= (cacheline.data[startByte + 1]) << 1 * 8;
        data |= (cacheline.data[startByte + 2]) << 2 * 8;
        data |= (cacheline.data[startByte + 3]) << 3 * 8;

        startWriteToRAM(data, addr);
        std::cout << "Wainting for RAM write " << std::endl;
        do {
            wait(clock.posedge_event());
            cyclesPassedInRequest++;
        } while ((!memoryReadyBus.read()));
        std::cout << "Done waiting for RAM write " << std::endl;
        endWriteToRAM();
    }

    void endWriteToRAM() {
        std::cout << "ENDING RAM WRITE REQUEST" << std::endl;
        memoryValidRequestBus.write(false);
    }

    // precondition: cacheline is in cache
    void startWriteToRAM(std::uint32_t data, std::uint32_t addr) {
        memoryAddrBus.write(addr);
        memoryWeBus.write(true);
        std::cout << "Writing true???" << std::endl;
        memoryDataOutBus.write(data);
        memoryValidRequestBus.write(true);
    }

    void startReadFromRAM(std::uint32_t addr) {
        std::uint32_t alignedAddr = (addr / cacheLineSize) * cacheLineSize;
        memoryAddrBus.write(alignedAddr);
        memoryWeBus.write(false);
        memoryValidRequestBus.write(true);
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
        std::find_if(cacheInternal.begin(), cacheInternal.end(), [&decomposedAddr](Cacheline& cacheline) {
            return cacheline.isUsed && cacheline.tag == decomposedAddr.tag;
        });
    return cachelineFoundTag;
}

template <>
inline std::vector<Cacheline>::iterator
Cache<MappingType::Direct>::chooseWhichCachelineToFillFromRAM(DecomposedAddress decomposedAddr) {
    auto cachelineToWriteInto = cacheInternal.begin() + decomposedAddr.index; // there is only one possible space
    assert(cachelineToWriteInto != cacheInternal.end());
    return cachelineToWriteInto;
}

template <>
inline std::vector<Cacheline>::iterator
Cache<MappingType::Fully_Associative>::chooseWhichCachelineToFillFromRAM(DecomposedAddress decomposedAddr) {
    auto firstUnusedCacheline = std::find_if(cacheInternal.begin(), cacheInternal.end(),
                                             [](const Cacheline& cacheline) { return !cacheline.isUsed; });
    if (firstUnusedCacheline == cacheInternal.end()) {
        firstUnusedCacheline = cacheInternal.begin() + replacementPolicy->pop();
    }
    assert(firstUnusedCacheline != cacheInternal.end());
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

template <MappingType mappingType> inline void Cache<mappingType>::waitForRAM() {
    std::cout << "Now waiting for RAM" << std::endl;
    do {
        wait(clock.posedge_event());
        cyclesPassedInRequest++;
    } while (!memoryReadyBus.read());
    std::cout << " Done waiting for RAM" << std::endl;
}