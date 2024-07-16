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

constexpr std::uint8_t RAM_READ_BUS_SIZE_IN_BYTE{16};
constexpr std::uint8_t BITS_IN_BYTE{8};
constexpr std::uint8_t WRITE_BUFFER_SIZE{4}; // chosen by fair dice roll. guaranteed to be optimal

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
    sc_core::sc_in<sc_dt::sc_bv<RAM_READ_BUS_SIZE_IN_BYTE * BITS_IN_BYTE>> SC_NAMED(memoryDataInBus);
    sc_core::sc_in<bool> SC_NAMED(memoryReadyBus);

    // Cache -> CPU
    sc_core::sc_out<bool> SC_NAMED(ready);
    sc_core::sc_out<std::uint32_t> SC_NAMED(cpuDataOutBus);

    // ========== Internal Signals  ==============
    // Buffer -> Cache
    sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS> SC_NAMED(writeBufferReady);
    sc_core::sc_signal<sc_dt::sc_bv<RAM_READ_BUS_SIZE_IN_BYTE * BITS_IN_BYTE>> SC_NAMED(writeBufferDataOut);

    // Cache -> Buffer
    sc_core::sc_signal<std::uint32_t> SC_NAMED(writeBufferAddr);
    sc_core::sc_signal<std::uint32_t> SC_NAMED(writeBufferDataIn);
    sc_core::sc_signal<bool> SC_NAMED(writeBufferWE);
    sc_core::sc_signal<bool> SC_NAMED(writeBufferValidRequest);

    // ========== Hit/Miss Bookkeeping  ==============
    std::uint64_t hitCount{0};
    std::uint64_t missCount{0};

  private:
    // ========== Config  ==============
    std::uint32_t numCacheLines{0};
    std::uint32_t cacheLineSize{0}; // in Byte
    std::uint32_t cacheLatency{0};  // in Cycles
    std::unique_ptr<ReplacementPolicy<std::uint32_t>> replacementPolicy{nullptr};

    // ========== Internals ==============
    std::vector<Cacheline> cacheInternal;
    WriteBuffer<WRITE_BUFFER_SIZE> writeBuffer;
    std::uint32_t cyclesPassedInRequest{0};

    // ========== Precomputation ==============
    std::uint8_t addressOffsetBits{0};
    std::uint8_t addressIndexBits{0};
    std::uint8_t addressTagBits{0};
    std::uint32_t addressOffsetBitMask{0};
    std::uint32_t addressIndexBitMask{0};
    std::uint32_t addressTagBitMask{0};

  public:
    Cache(sc_core::sc_module_name name, std::uint32_t numCacheLines, std::uint32_t cacheLineSize,
          std::uint32_t cacheLatency, std::unique_ptr<ReplacementPolicy<std::uint32_t>> policy);

  private:
    // ========== Set-Up ==============
    SC_CTOR(Cache); // private since this is never to be called, just to get systemc typedef
    void setUpWriteBufferConnects();
    void zeroInitialiseCachelines();
    constexpr void precomputeAddressDecompositionBits() noexcept;

    // ========== Main Request Handling ==============
    void handleRequest();
    void handleSubRequest(SubRequest subRequest, std::uint32_t & readData);
    Request constructRequestFromBusses();

    // ========== Helpers to determine which cache line to read from / write to ==============
    constexpr DecomposedAddress decomposeAddress(std::uint32_t address) noexcept;
    std::vector<Cacheline>::iterator getCachelineOwnedByAddr(DecomposedAddress decomposedAddr);
    std::vector<Cacheline>::iterator chooseWhichCachelineToFillFromRAM(DecomposedAddress decomposedAddr);
    void registerUsage(std::vector<Cacheline>::iterator cacheline);

    // ========== Reading from Cache ==============
    std::vector<Cacheline>::iterator fetchIfNotPresent(std::uint32_t addr, DecomposedAddress decomposedAddr);
    void startReadFromRAM(std::uint32_t addr);
    std::uint32_t doRead(DecomposedAddress decomposedAddr, Cacheline & cacheline, std::uint8_t numBytes);
    std::vector<Cacheline>::iterator writeRAMReadIntoCacheline(DecomposedAddress decomposedAddr);

    // ========== Writing to Cache ==============
    void doWrite(Cacheline & cacheline, DecomposedAddress decomposedAddr, std::uint32_t data, std::uint8_t numBytes);
    void passWriteOnToRAM(Cacheline & cacheline, DecomposedAddress decomposedAddr, std::uint32_t addr);

    // ========== Waiting Helpers ==============
    template <typename Predicate> void waitUntilTrue(Predicate pred, bool waitOneCycleBeforeCheck);
    void waitOutCacheLatency();
    void waitForRAM();
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
    return std::find_if(cacheInternal.begin(), cacheInternal.end(), [&decomposedAddr](Cacheline& cacheline) {
        return cacheline.isUsed && cacheline.tag == decomposedAddr.tag;
    });
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
    waitUntilTrue([&writeBufferReady = writeBufferReady]() { return writeBufferReady.read(); }, true);
    std::cout << " Done waiting for WriteBuffer" << std::endl;
}

template <> constexpr inline void Cache<MappingType::Fully_Associative>::precomputeAddressDecompositionBits() noexcept {
    addressOffsetBits = safeCeilLog2(cacheLineSize);
    addressIndexBits = 0; // no index bits in fully associative cache
    addressTagBits = 32 - addressOffsetBits;

    addressOffsetBitMask = generateBitmaskForLowestNBits(addressOffsetBits);
    addressTagBitMask = generateBitmaskForLowestNBits(addressTagBits);
}

template <> constexpr inline void Cache<MappingType::Direct>::precomputeAddressDecompositionBits() noexcept {
    addressOffsetBits = safeCeilLog2(cacheLineSize);
    addressIndexBits = safeCeilLog2(numCacheLines);
    addressTagBits = 32 - addressIndexBits - addressOffsetBits;

    addressOffsetBitMask = generateBitmaskForLowestNBits(addressOffsetBits);
    addressIndexBitMask = generateBitmaskForLowestNBits(addressIndexBits);
    addressTagBitMask = generateBitmaskForLowestNBits(addressTagBits);
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
      cacheLatency{cacheLatency}, cacheInternal{numCacheLines},
      writeBuffer{"writeBuffer", cacheLineSize / RAM_READ_BUS_SIZE_IN_BYTE} {

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
    assert(cachelineToWriteInto->data.size() % RAM_READ_BUS_SIZE_IN_BYTE == 0);
    sc_dt::sc_bv<RAM_READ_BUS_SIZE_IN_BYTE * BITS_IN_BYTE> dataRead;
    std::uint32_t numReadEvents = (cachelineToWriteInto->data.size() / RAM_READ_BUS_SIZE_IN_BYTE);

    // dont need to wait before first one because we can only get here if RAM tells us it is ready
    for (int i = 0; i < numReadEvents; ++i) {
        dataRead = writeBufferDataOut.read();
        std::cout << "Cache received: " << writeBufferDataOut.read() << std::endl;

        for (int byte = 0; byte < RAM_READ_BUS_SIZE_IN_BYTE; ++byte) {
            cachelineToWriteInto->data[RAM_READ_BUS_SIZE_IN_BYTE * i + byte] =
                dataRead.range(BITS_IN_BYTE * byte + (BITS_IN_BYTE - 1), BITS_IN_BYTE * byte).to_uint();
            std::cout << dataRead.range(BITS_IN_BYTE * byte + (BITS_IN_BYTE - 1), BITS_IN_BYTE * byte).to_uint() << "/"
                      << cachelineToWriteInto->data[RAM_READ_BUS_SIZE_IN_BYTE * i + byte] << " ";
        }
        std::cout << std::endl;
        // if this is the last one we don't need to wait anymore
        if (i + 1 <= numReadEvents)
            wait(clock.posedge_event());
    }
    writeBufferValidRequest.write(false);
    // wait(clock.posedge_event()); // DEBUG
    // wait(clock.posedge_event()); // DEBUG
    std::cout << "Unsetting valid request now" << std::endl;
    cachelineToWriteInto->isUsed = true;
    cachelineToWriteInto->tag = decomposedAddr.tag;

    std::cout << "Done writing RAM Read into cacheline :" << *cachelineToWriteInto << std::endl;

    return cachelineToWriteInto;
}

template <MappingType mappingType> inline void Cache<mappingType>::waitOutCacheLatency() {
    waitUntilTrue(
        [cycles = 0ull, cacheLatency = cacheLatency]() mutable {
            if (cycles < cacheLatency) {
                cycles++;
                return false;
            }
            return true;
        },
        false);
}

template <MappingType mappingType>
inline std::vector<Cacheline>::iterator Cache<mappingType>::fetchIfNotPresent(std::uint32_t addr,
                                                                              DecomposedAddress decomposedAddr) {
    waitOutCacheLatency();

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
    // std::cout << "Starting handling subrequest " << subRequest.addr << " " << subRequest.data << " "
    //         << unsigned(subRequest.size) << " " << subRequest.we << " " << unsigned(subRequest.bitsBefore)
    //       << std::endl;

    // std::cout << "Potentially reading into cache if not present...\n";
    auto addr = subRequest.addr;

    // split into tag - index - offset
    auto decomposedAddr = decomposeAddress(addr);
    // check if in cache (waits for #cycles specified by cacheLatency) - if not read from RAM

    std::cout << "Curr tag: " << decomposedAddr.tag << std::endl;
    std::cout << "Curr index " << decomposedAddr.index << " offset " << decomposedAddr.offset << std::endl;
    std::cout << "Tags in cache:" << std::endl;
    for (auto& cachl : cacheInternal) {
        std::cout << cachl.tag << "\n";
    }

    auto cacheline = fetchIfNotPresent(addr, decomposedAddr);

    // std::cout << "Done reading into cache\n";

    // if a fully associative Cache and we use a stateful policy we declare the use of the cacheline here. In Direct
    // Mapped cache this is a NOP
    registerUsage(cacheline);
    // std::cout << "Cacheline we read looks like: " << *cacheline << std::endl;

    if (subRequest.we) {
        doWrite(*cacheline, decomposedAddr, subRequest.data, subRequest.size);
        passWriteOnToRAM(*cacheline, decomposedAddr, addr);
        // std::cout << "After writing it now looks like: " << *cacheline << std::endl;
    } else {
        auto tempReadData = doRead(decomposedAddr, *cacheline, subRequest.size);
        // std::cout << "Just read " << tempReadData << " from cache\n";
        readData = applyPartialRead(subRequest, readData, tempReadData);
    }

    // std::cout << "Done with subcycle" << std::endl;
}

template <MappingType mappingType>
inline std::uint32_t Cache<mappingType>::doRead(DecomposedAddress decomposedAddr, Cacheline& cacheline,
                                                std::uint8_t numBytes) {
    std::cout << unsigned(numBytes) << " " << decomposedAddr.offset << std::endl;
    assert((numBytes + decomposedAddr.offset - 1) < cacheLineSize);

    std::uint32_t retVal = 0;
    for (std::size_t byteNr = 0; byteNr < numBytes; ++byteNr) {
        retVal += cacheline.data.at(decomposedAddr.offset + byteNr) << (byteNr * BITS_IN_BYTE);
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

        // while this is also passed into write requests, it is only relevant for read request and will not be accessed
        // if invalid for the request type
        std::uint32_t readData = 0;

        for (auto& subRequest : subRequests) {
            handleSubRequest(std::move(subRequest), readData);
        }

        if (!request.we) {
            std::cout << "Sending " << readData << " back to CPU" << std::endl;
            cpuDataOutBus.write(readData);
        }

        std::cout << "Done with cycle" << std::endl;

        // it is the responsibility of the CPU to have stopped the valid request signal at the latest during the cycle
        // he gets this signal. To not still read the valid request signal from the previous cycle we sleep for one and
        // only then start checking again
        ready.write(true);
        wait();
    }
}

template <MappingType mappingType>
inline void Cache<mappingType>::doWrite(Cacheline& cacheline, DecomposedAddress decomposedAddr, std::uint32_t data,
                                        std::uint8_t numBytes) {
    std::cout << unsigned(numBytes) << " " << decomposedAddr.offset << std::endl;

    assert((numBytes + decomposedAddr.offset - 1) < cacheLineSize);
    std::cout << "Attempting to write " << data << std::endl;
    for (std::size_t byteNr = 0; byteNr < numBytes; ++byteNr) {
        cacheline.data[(decomposedAddr.offset + byteNr)] =
            (data >> BITS_IN_BYTE * byteNr) & generateBitmaskForLowestNBits(BITS_IN_BYTE);
    }
    std::cout << "Done writing into cacheline" << std::endl;
}

template <MappingType mappingType>
inline void Cache<mappingType>::passWriteOnToRAM(Cacheline& cacheline, DecomposedAddress decomposedAddr,
                                                 std::uint32_t addr) {
    std::uint32_t data = 0;

    std::size_t startByte = std::min(static_cast<std::size_t>(decomposedAddr.offset), cacheline.data.size() - 4);

    data |= (cacheline.data[startByte + 0]) << 0 * BITS_IN_BYTE;
    data |= (cacheline.data[startByte + 1]) << 1 * BITS_IN_BYTE;
    data |= (cacheline.data[startByte + 2]) << 2 * BITS_IN_BYTE;
    data |= (cacheline.data[startByte + 3]) << 3 * BITS_IN_BYTE;

    std::cout << "Passing to write buffer" << std::endl;
    writeBufferAddr.write(addr);
    writeBufferDataIn.write(data);
    writeBufferWE.write(true);
    // wait(clock.posedge_event()); // DEBUG
    // wait(clock.posedge_event()); // DEBUG
    writeBufferValidRequest.write(true);
    // wait(clock.posedge_event()); // DEBUG
    // wait(clock.posedge_event()); // DEBUG
    // write buffer takes at least one cycle (DEFINITION)
    do {
        wait(clock.posedge_event());
        cyclesPassedInRequest++;
    } while (!writeBufferReady);

    writeBufferValidRequest.write(false);
    // wait(clock.posedge_event()); // DEBUG
    // wait(clock.posedge_event()); // DEBUG
    std::cout << "Passed to write buffer " << std::endl;
}

template <MappingType mappingType> inline void Cache<mappingType>::startReadFromRAM(std::uint32_t addr) {
    std::uint32_t alignedAddr = (addr / cacheLineSize) * cacheLineSize;
    writeBufferAddr.write(alignedAddr);
    writeBufferWE.write(false);
    std::cout << "Setting WB valid req to true in read " << std::endl;
    // wait(clock.posedge_event()); // DEBUG
    writeBufferValidRequest.write(true);
}

template <MappingType mappingType> inline Request Cache<mappingType>::constructRequestFromBusses() {
    return Request{cpuAddrBus.read(), cpuDataInBus.read(), cpuWeBus.read()};
}

template <MappingType mappingType>
template <typename Predicate>
inline void Cache<mappingType>::waitUntilTrue(Predicate pred, bool waitOneCycleBeforeCheck) {
    if (waitOneCycleBeforeCheck) {
        wait(clock.posedge_event());
    }
    while (!pred()) {
        wait(clock.posedge_event());
    }
}