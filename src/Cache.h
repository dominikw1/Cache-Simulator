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

constexpr std::uint16_t RAM_READ_BUS_SIZE_IN_BYTE{16}; // NOT a config value, just a transparent way to access
constexpr std::uint16_t BITS_IN_BYTE{8};
constexpr std::uint16_t WRITE_BUFFER_SIZE{4}; // chosen by fair dice roll. guaranteed to be optimal :)

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
    /**
     * Constructs a write-buffering cache.
     * @param[in] name  The name systemc assigns to this module.
     * @param[in] numCacheLines The number of cache lines the cache will have. Has to be > 0.
     * @param[in] cacheLineSize The number of bytes a cacheline holds. Has to be a multiple of the memory bus size 16B
     * and > 0.
     * @param[in] cacheLatency The number of cycles the cache takes to find out whether an access results in a hit or a
     * miss. Mind that the total number of cycles until data has been fully transfered is strictly larger than this
     * value, as the transferring takes some cycles itself.
     * @param[in] policy Optional parameter - the replacement policy taking effect when the cache is full and a new
     * entry shall be stored. Only relevant if MappingType is Fully_Associative, results in a warning on Direct if not
     * null_ptr. Takes ownership of the policy. Default value is nullptr.
     *
     */
    Cache(sc_core::sc_module_name name, std::uint32_t numCacheLines, std::uint32_t cacheLineSize,
          std::uint32_t cacheLatency, std::unique_ptr<ReplacementPolicy<std::uint32_t>> policy = nullptr) noexcept;
    /**
     * Approximates the primitive gate count used to construct this cache
     */
    std::size_t calculateGateCount() const;

  private:
    // ========== Set-Up ==============
    SC_CTOR(Cache); // private since this is never to be called, just to get systemc typedef

    /**
     * Connect the internal write buffer to the external ports of the cache and internal signals used to communicate
     * with it.
     */
    constexpr void setUpWriteBufferConnects() noexcept;
    /**
     * Initialise all cachelines with 0 bytes.
     */
    constexpr void zeroInitialiseCachelines() noexcept;
    /**
     * Precomputes what (and how many) bits of an adress correspond to tag, index and offset in our cache. Furthermore
     * preconstructs bit masks to extract those values.
     */
    constexpr void precomputeAddressDecompositionBits() noexcept;

    // ========== Main Request Handling ==============
    /**
     * The main point of entry for each request. The request gets split up into non-cacheline crossing subrequests and
     * the appropriate signals to the outside world are set
     */
    constexpr void handleRequest() noexcept;
    /**
     * Handles the cacheline-internal requests and controls the actual performance like loading the cacheline from RAM
     * if not present, controlling the writes and reads, etc.
     */
    constexpr void handleSubRequest(SubRequest subRequest, std::uint32_t & readData) noexcept;
    /**
     * Constructs a request object by reading the busses written to by the CPU for easier internal handling.
     */
    constexpr Request constructRequestFromBusses() const noexcept;

    // ========== Helpers to determine which cache line to read from / write to ==============
    /**
     * Use precomputed masks to decompose address into tag, index and offset
     */
    constexpr DecomposedAddress decomposeAddress(std::uint32_t address) noexcept;
    /**
     * Use address decomposed into tag, index and offset to find a cacheline in the cache that is already "owned" by
     * this address, meaning the tag matches and it is a valid cacheline. How this cacheline is found is determined by
     * the Mapping Type
     */
    std::vector<Cacheline>::iterator getCachelineOwnedByAddr(DecomposedAddress decomposedAddr) noexcept;
    /**
     * Determine which cacheline the read from RAM shall be read into. How this is chosen depends on the MappingType
     */
    std::vector<Cacheline>::iterator chooseWhichCachelineToFillFromRAM(DecomposedAddress decomposedAddr);
    /**
     * If this is a fully associative cache with a stateful policy (e.g. LRU), this updates the aforementioned state. If
     * direct mapped, this is a NOP
     */
    void registerUsage(std::vector<Cacheline>::iterator cacheline) noexcept;

    // ========== Reading from Cache ==============
    /**
     * Determines whether we have a cache hit or not and fetches the cacheline from RAM if it's a miss
     */
    constexpr std::vector<Cacheline>::iterator fetchIfNotPresent(std::uint32_t addr,
                                                                 DecomposedAddress decomposedAddr) noexcept;
    /**
     * Sends request to RAM through Write Buffer to read in cacheline.
     * @param[in] addr Cacheline-size aligned addr for the first byte to be read
     */
    constexpr void startReadFromRAM(std::uint32_t addr) noexcept;
    /**
     * Reads a data segment from cacheline
     * @param[in] decomposedAddr the address decomposed into tag, index and offset
     * @param[in] cacheline The cacheline to be read from
     * @param[in] numBytes the amount of bytes we want to read
     * @returns the data segment just read in the lowest numBytes bytes of the uint32_t
     * */
    constexpr std::uint32_t doRead(DecomposedAddress decomposedAddr, Cacheline & cacheline, std::uint8_t numBytes) noexcept;
    /**
     * Reads data from bus written to by RAM and copies it into the corresponding cacheline
     * */
    constexpr std::vector<Cacheline>::iterator writeRAMReadIntoCacheline(DecomposedAddress decomposedAddr) noexcept;

    // ========== Writing to Cache ==============
    void doWrite(Cacheline & cacheline, DecomposedAddress decomposedAddr, std::uint32_t data, std::uint8_t numBytes);
    void passWriteOnToRAM(Cacheline & cacheline, DecomposedAddress decomposedAddr, std::uint32_t addr);

    // ========== Waiting Helpers ==============
    constexpr void waitOutCacheLatency() noexcept;
    void waitForRAM();
};
