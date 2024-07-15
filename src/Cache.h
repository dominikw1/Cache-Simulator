#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>

#include <systemc>

#include "Cacheline.h"
#include "DecomposedAddress.h"
#include "ReplacementPolicy.h"
#include "Request.h"
#include "Result.h"
#include "SubRequest.h"
#include "WriteBuffer.h"

enum class MappingType { Direct, Fully_Associative };

template <MappingType mappingType> SC_MODULE(Cache) {
  public:
    // ========== External Ports  ==============
    // Global Clock
    sc_core::sc_in<bool> SC_NAMED(clock);

    // CPU -> Cache
    sc_core::sc_in<std::uint32_t> SC_NAMED(cpuAddrBus);
    sc_core::sc_in<std::uint32_t> SC_NAMED(cpuDataInBus);
    sc_core::sc_in<bool> SC_NAMED(cpuWeBus);
    sc_core::sc_in<bool> SC_NAMED(cpuValidRequest);

    // Cache -> RAM
    sc_core::sc_out<std::uint32_t> SC_NAMED(memoryAddrBus);
    sc_core::sc_out<std::uint32_t> SC_NAMED(memoryDataOutBus);
    sc_core::sc_out<bool> SC_NAMED(memoryWeBus);
    sc_core::sc_out<bool> SC_NAMED(memoryValidRequestBus);

    // RAM -> Cache
    sc_core::sc_in<sc_dt::sc_bv<128>> SC_NAMED(memoryDataInBus);
    sc_core::sc_in<bool> SC_NAMED(memoryReadyBus);

    // Cache -> CPU
    sc_core::sc_out<bool> SC_NAMED(ready);
    sc_core::sc_out<std::uint32_t> SC_NAMED(cpuDataOutBus);

    // ========== Internal Signals  ==============
    // Buffer -> Cache
    sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS> SC_NAMED(writeBufferReady);
    sc_core::sc_signal<sc_dt::sc_bv<128>> SC_NAMED(writeBufferDataOut);

    // Cache -> Buffer
    sc_core::sc_signal<std::uint32_t> SC_NAMED(writeBufferAddr);
    sc_core::sc_signal<std::uint32_t> SC_NAMED(writeBufferDataIn);
    sc_core::sc_signal<bool> SC_NAMED(writeBufferWE);
    sc_core::sc_signal<bool> SC_NAMED(writeBufferValidRequest);

    // ========== Hit/Miss Bookkeeping  ==============
    std::uint64_t hitCount = 0;
    std::uint64_t missCount = 0;

  private:
    // ========== Config  ==============
    std::uint64_t numCacheLines = 0; // in Byte
    std::uint64_t cacheLineSize = 0; // in Byte
    std::uint64_t cacheLatency = 0;  // in Cycles
    std::unique_ptr<ReplacementPolicy<std::uint32_t>> replacementPolicy = nullptr;

    // ========== Internals ==============
    std::vector<Cacheline> cacheInternal;
    WriteBuffer<4> writeBuffer;
    std::uint32_t cyclesPassedInRequest = 0;

    // ========== Precomputation ==============
    std::uint8_t addressOffsetBits = 0;
    std::uint8_t addressIndexBits = 0;
    std::uint8_t addressTagBits = 0;
    std::uint32_t addressOffsetBitMask = 0;
    std::uint32_t addressIndexBitMask = 0;
    std::uint32_t addressTagBitMask = 0;

  public:
    Cache(sc_core::sc_module_name name, unsigned int numCacheLines, unsigned int cacheLineSize,
          unsigned int cacheLatency, std::unique_ptr<ReplacementPolicy<std::uint32_t>> policy);

  private:
    // ========== Set-Up ==============
    SC_CTOR(Cache); // private since this is never to be called, just to get systemc typedef
    void setUpWriteBufferConnects();
    void zeroInitialiseCachelines();

    // ========== Main Request Handling ==============
    void handleRequest();
    void handleSubRequest(SubRequest subRequest, std::uint32_t & readData);
    Request constructRequestFromBusses();

    // ========== Helpers to determine which cache line to read from / write to ==============
    constexpr void precomputeAddressDecompositionBits() noexcept;
    constexpr DecomposedAddress decomposeAddress(std::uint32_t address) noexcept;
    std::vector<Cacheline>::iterator getCachelineOwnedByAddr(DecomposedAddress decomposedAddr);
    std::vector<Cacheline>::iterator chooseWhichCachelineToFillFromRAM(DecomposedAddress decomposedAddr);
    void registerUsage(std::vector<Cacheline>::iterator cacheline);

    // ========== Reading from Cache ==============
    std::vector<Cacheline>::iterator fetchIfNotPresent(std::uint32_t addr, DecomposedAddress decomposedAddr);
    void startReadFromRAM(std::uint32_t addr);
    std::uint32_t doRead(DecomposedAddress decomposedAddr, Cacheline & cacheline, std::uint8_t numBytes);
    std::vector<Cacheline>::iterator writeRAMReadIntoCacheline(DecomposedAddress decomposedAddr);
    void waitForRAM();

    // ========== Writing to Cache ==============
    void doWrite(Cacheline & cacheline, DecomposedAddress decomposedAddr, std::uint32_t data, std::uint8_t numBytes);
    void passWriteOnToRAM(Cacheline & cacheline, DecomposedAddress decomposedAddr, std::uint32_t addr);
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
    assert(addressOffsetBitMask > 0 && addressIndexBitMask > 0 && addressTagBitMask > 0);
    return DecomposedAddress{((address >> addressOffsetBits) >> addressIndexBits) & addressTagBitMask,
                             (address >> addressOffsetBits) & addressIndexBitMask, address & addressOffsetBitMask};
}

template <>
inline constexpr DecomposedAddress
Cache<MappingType::Fully_Associative>::decomposeAddress(std::uint32_t address) noexcept {
    assert(addressOffsetBitMask > 0 && addressIndexBitMask == 0 && addressTagBitMask > 0);
    return DecomposedAddress{(address >> addressOffsetBits) & addressTagBitMask, 0, address & addressOffsetBitMask};
}

template <> inline void Cache<MappingType::Direct>::registerUsage(std::vector<Cacheline>::iterator cacheline) {
    // no bookeeping needed
}

template <>
inline void Cache<MappingType::Fully_Associative>::registerUsage(std::vector<Cacheline>::iterator cacheline) {
    replacementPolicy->logUse(cacheline - cacheInternal.begin());
}

template <MappingType mappingType> inline void Cache<mappingType>::waitForRAM() {
    std::cout << "Now waiting for WriteBuffer" << std::endl;
    do {
        wait(clock.posedge_event());
        cyclesPassedInRequest++;
    } while (!writeBufferReady.read());
    std::cout << " Done waiting for WriteBuffer" << std::endl;
}

template <> constexpr inline void Cache<MappingType::Fully_Associative>::precomputeAddressDecompositionBits() noexcept {
    addressOffsetBits = safeCeilLog2(cacheLineSize);
    addressIndexBits = 0;
    addressTagBits = 32 - addressOffsetBits;

    addressOffsetBitMask = (1ll << addressOffsetBits) - 1;
    addressTagBitMask = (1ll << addressTagBits) - 1;
}

template <> constexpr inline void Cache<MappingType::Direct>::precomputeAddressDecompositionBits() noexcept {
    addressOffsetBits = safeCeilLog2(cacheLineSize);
    addressIndexBits = safeCeilLog2(numCacheLines);
    addressTagBits = 32 - addressIndexBits - addressOffsetBits;

    addressOffsetBitMask = (1ll << addressOffsetBits) - 1;
    addressIndexBitMask = (1ll << addressIndexBits) - 1;
    addressTagBitMask = (1ll << addressTagBits) - 1;
}

template <MappingType mappingType> inline void Cache<mappingType>::setUpWriteBufferConnects() {
    writeBuffer.clock.bind(clock);

    writeBuffer.ready.bind(writeBufferReady);
    writeBuffer.cacheDataOutBus.bind(writeBufferDataOut);

    writeBuffer.cacheAddrBus.bind(writeBufferAddr);
    writeBuffer.cacheDataInBus.bind(writeBufferDataIn);
    writeBuffer.cacheWeBus.bind(writeBufferWE);
    writeBuffer.cacheValidRequest.bind(writeBufferValidRequest);

    writeBuffer.memoryAddrBus.bind(memoryAddrBus);
    writeBuffer.memoryDataOutBus.bind(memoryDataOutBus);
    writeBuffer.memoryWeBus.bind(memoryWeBus);
    writeBuffer.memoryValidRequestBus.bind(memoryValidRequestBus);

    writeBuffer.memoryDataInBus.bind(memoryDataInBus);
    writeBuffer.memoryReadyBus.bind(memoryReadyBus);
}

template <MappingType mappingType> inline void Cache<mappingType>::zeroInitialiseCachelines() {
    for (auto& cacheline : cacheInternal) {
        cacheline.data = std::vector<std::uint8_t>(cacheLineSize, 0);
    }
}

template <MappingType mappingType>
inline Cache<mappingType>::Cache(sc_core::sc_module_name name, unsigned int numCacheLines, unsigned int cacheLineSize,
                                 unsigned int cacheLatency, std::unique_ptr<ReplacementPolicy<std::uint32_t>> policy)
    : sc_module{name}, replacementPolicy{std::move(policy)}, numCacheLines{numCacheLines}, cacheLineSize{cacheLineSize},
      cacheLatency{cacheLatency}, cacheInternal{numCacheLines}, writeBuffer{"writeBuffer", cacheLineSize / 16} {

    using namespace sc_core; // in scope as to not pollute global namespace
    zeroInitialiseCachelines();
    precomputeAddressDecompositionBits();
    setUpWriteBufferConnects();

    SC_THREAD(handleRequest);
    sensitive << clock.pos();
}

template <MappingType mappingType>
inline std::vector<Cacheline>::iterator
Cache<mappingType>::writeRAMReadIntoCacheline(DecomposedAddress decomposedAddr) {
    std::cout << "Writing RAM Read into cacheline " << std::endl;
    auto cachelineToWriteInto = chooseWhichCachelineToFillFromRAM(decomposedAddr);

    // we do not allow any inputs violating this rule
    assert(cachelineToWriteInto->data.size() % 16 == 0);
    sc_dt::sc_bv<128> dataRead;
    std::uint32_t numReadEvents = (cachelineToWriteInto->data.size() / 16);

    //        throw std::runtime_error("actually reading??");
    // dont need to wait before first one because we can only get here if RAM tells us it is ready
    for (int i = 0; i < numReadEvents; ++i) {
        dataRead = writeBufferDataOut.read();

        // 128 bits -> 16 bytes
        for (int byte = 0; byte < 16; ++byte) {
            cachelineToWriteInto->data[16 * i + byte] = dataRead.range(16 * i + 15, 16 * i).to_uint();
        }

        // if this is the last one we don't need to wait anymore
        if (i + 1 <= numReadEvents)
            wait(clock.posedge_event());
    }
    writeBufferValidRequest.write(false);
    std::cout << "Unsetting valid request now" << std::endl;
    cachelineToWriteInto->isUsed = true;
    cachelineToWriteInto->tag = decomposedAddr.tag;

    std::cout << "Done writing RAM Read into cacheline " << std::endl;
    return cachelineToWriteInto;
}

template <MappingType mappingType>
inline std::vector<Cacheline>::iterator Cache<mappingType>::fetchIfNotPresent(std::uint32_t addr,
                                                                              DecomposedAddress decomposedAddr) {
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

template <MappingType mappingType>
inline void Cache<mappingType>::handleSubRequest(SubRequest subRequest, std::uint32_t& readData) {
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

template <MappingType mappingType>
inline std::uint32_t Cache<mappingType>::doRead(DecomposedAddress decomposedAddr, Cacheline& cacheline,
                                                std::uint8_t numBytes) {
    std::cout << unsigned(numBytes) << " " << decomposedAddr.offset << std::endl;
    assert((numBytes + decomposedAddr.offset - 1) < cacheLineSize);

    std::uint32_t retVal = 0;
    for (std::size_t byteNr = 0; byteNr < numBytes; ++byteNr) {
        retVal += cacheline.data.at(decomposedAddr.offset + byteNr) << (byteNr * 8);
    }
    return retVal;
}

template <MappingType mappingType> inline void Cache<mappingType>::handleRequest() {
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

template <MappingType mappingType>
inline void Cache<mappingType>::doWrite(Cacheline& cacheline, DecomposedAddress decomposedAddr, std::uint32_t data,
                                        std::uint8_t numBytes) {
    std::cout << unsigned(numBytes) << " " << decomposedAddr.offset << std::endl;

    assert((numBytes + decomposedAddr.offset - 1) < cacheLineSize);
    std::cout << "Attempting to write " << data << std::endl;
    for (std::size_t byteNr = 0; byteNr < numBytes; ++byteNr) {
        cacheline.data[(decomposedAddr.offset + byteNr)] = (data >> 8 * byteNr) & ((1 << 8) - 1);
    }
    std::cout << "Done writing into cacheline" << std::endl;
}

template <MappingType mappingType>
inline void Cache<mappingType>::passWriteOnToRAM(Cacheline& cacheline, DecomposedAddress decomposedAddr,
                                                 std::uint32_t addr) {
    std::uint32_t data = 0;

    std::size_t startByte = std::min(static_cast<std::size_t>(decomposedAddr.offset), cacheline.data.size() - 4);

    data |= (cacheline.data[startByte + 0]) << 0 * 8;
    data |= (cacheline.data[startByte + 1]) << 1 * 8;
    data |= (cacheline.data[startByte + 2]) << 2 * 8;
    data |= (cacheline.data[startByte + 3]) << 3 * 8;

    std::cout << "Passing to write buffer" << std::endl;
    writeBufferAddr.write(addr);
    writeBufferDataIn.write(data);
    writeBufferWE.write(true);
    writeBufferValidRequest.write(true);

    // write buffer takes at least one cycle (DEFINITION)
    do {
        wait(clock.posedge_event());
        cyclesPassedInRequest++;
    } while (!writeBufferReady);

    writeBufferValidRequest.write(false);
    std::cout << "Passed to write buffer " << std::endl;
}

template <MappingType mappingType> inline void Cache<mappingType>::startReadFromRAM(std::uint32_t addr) {
    std::uint32_t alignedAddr = (addr / cacheLineSize) * cacheLineSize;
    writeBufferAddr.write(alignedAddr);
    writeBufferWE.write(false);
    std::cout << "Setting WB valid req to true in read " << std::endl;
    writeBufferValidRequest.write(true);
}

template <MappingType mappingType> inline Request Cache<mappingType>::constructRequestFromBusses() {
    return Request{cpuAddrBus.read(), cpuDataInBus.read(), cpuWeBus.read()};
}
