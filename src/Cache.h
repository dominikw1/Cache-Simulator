#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>

#include <systemc>

#include "Cacheline.h"
#include "DecomposedAddress.h"
#include "Policy/ReplacementPolicy.h"
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
          std::uint32_t cacheLatency, std::unique_ptr<ReplacementPolicy<std::uint32_t>> policy = nullptr);

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
    void waitOutCacheLatency();
    void waitForRAM();
};
