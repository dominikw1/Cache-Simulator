
#ifndef CACHE_HPP
#define CACHE_HPP

#include <systemc>

#include "Request.h"
#include "Result.h"
#include "Memory.h"
#include "CacheInternal.h"
#include "LRUPolicy.h"

enum class MappingType {
    DIRECT_MAPPED, FULL_ASSOCIATIVE
};

template<MappingType mappingType>
SC_MODULE(CACHE) {
public:
    sc_core::sc_in<uint32_t> cacheAddressBus;
    sc_core::sc_in<uint32_t> cacheDataBus;
    sc_core::sc_in<uint32_t> cacheWeBus;

    sc_core::sc_out<size_t> hit;
    sc_core::sc_out<size_t> miss;

    SC_CTOR(CACHE);

private:
    sc_core::sc_signal<uint32_t> memoryAddressBus;
    sc_core::sc_signal<uint32_t> memoryDataBus;
    sc_core::sc_signal<int> memoryWeBus;

    size_t hitCount;
    size_t missCount;

    MEMORY memory;
    CacheInternal cacheInternal;
    LRUPolicy<uint32_t> lru;

    unsigned int latency;

    unsigned int cacheLines;
    unsigned int cacheLineSize;

    std::vector<uint32_t> tags;

public:
    CACHE(sc_core::sc_module_name name, unsigned int cacheLines, unsigned int cacheLineSize,
          unsigned int cacheLatency, unsigned int memoryLatency)
            : sc_module{name},
              memory{"memory", memoryLatency},
              cacheInternal(cacheLines, cacheLineSize),
              latency{cacheLatency},
              cacheLines{cacheLines},
              tags{cacheLines} {

        memory.weBus(memoryWeBus);
        memory.dataBus(memoryDataBus);
        memory.addressBus(memoryAddressBus);

        SC_THREAD(update);
        sensitive << cacheWeBus;
    }

private:
    uint32_t getTag(uint32_t address);

    uint32_t getIndex(uint32_t address);

    void update() {
        if (cacheWeBus) {
            write(cacheAddressBus);
        } else {
            read(cacheAddressBus, cacheDataBus);
        }
    }

    uint32_t read(uint32_t address) {
        // TODO: add latency

        auto tag = getTag(address);
        auto index = getIndex(address);
        auto offset = getOffset(address);

        if (tags[index] != tag) {
            miss = ++missCount;

            memoryAddressBus = address;
            memoryWeBus = 0;

            // TODO: how to handle latency from memory?

            tags[index] = tag;
            auto dataFromMemory = memoryDataBus.read();
            auto dataFromMemoryInBytes = splitUpData(dataFromMemory);
            cacheInternal.handleRAMRead(InternalRequestMemoryRead{index, dataFromMemoryInBytes});
            free(dataFromMemoryInBytes);
        } else {
            hit = ++hitCount;
        }

        return cacheInternal.handleCPURead(InternalRequestCPURead{index, offset});
    }

    void write(uint32_t address, uint32_t data) {
        // TODO: add latency -> maybe not needed due to calling read anyway

        auto index = getIndex(address);
        auto offset = getOffset(address);

        read(address);

        // Write to cache
        cacheInternal.handleCPUWrite(InternalRequestCPUWrite{index, offset, data});

        // Write to memory
        memoryAddressBus = address;
        memoryDataBus = data;
        memoryWeBus = 1;
    }

    uint32_t getOffset(uint32_t address) {
        auto offsetBits = (int) log(cacheLineSize);

        auto offsetMask = (uint32_t) ((1 << offsetBits) - 1);
        return address & offsetMask;
    }

    uint8_t* splitUpData(uint32_t data) {
        uint8_t* bytes = (uint8_t*) malloc(4 * sizeof(uint8_t));

        bytes[0] = (data >> 0) & ((1 << 8) - 1);
        bytes[1] = (data >> 8) & ((1 << 8) - 1);
        bytes[2] = (data >> 16) & ((1 << 8) - 1);
        bytes[3] = (data >> 24) & ((1 << 8) - 1);

        return bytes;
    }
};

template<>
uint32_t CACHE<MappingType::DIRECT_MAPPED>::getTag(uint32_t address) {
    auto offsetBits = (int) log(cacheLineSize);

    auto tagMasks = (uint32_t) (-1 << offsetBits);
    return address & tagMasks;
}

template<>
uint32_t CACHE<MappingType::FULL_ASSOCIATIVE>::getTag(uint32_t address) {
    auto offsetBits = (int) log(cacheLineSize);
    auto indexBits = (int) log(cacheLines);

    auto tagMasks = (uint32_t) (-1 << (indexBits + offsetBits));
    return address & tagMasks;
}

template<>
uint32_t CACHE<MappingType::DIRECT_MAPPED>::getIndex(uint32_t address) {
    auto tag = getTag(address);

    // Search for tag
    auto temp = std::find(tags.begin(), tags.end(), tag);
    if (temp != tags.end()) {
        return (uint32_t) (temp - tags.begin());
    }

    // Search for next empty value
    temp = std::find(tags.begin(), tags.end(), 0);
    if (temp != tags.end()) {
        return (uint32_t) (temp - tags.begin());
    }

    // If cache full -> use LRU logic
    return lru.pop();
}

template<>
uint32_t CACHE<MappingType::FULL_ASSOCIATIVE>::getIndex(uint32_t address) {
    auto indexBits = (int) log(cacheLines);
    auto offsetBits = (int) log(cacheLineSize);

    auto indexMask = (uint32_t) ((1 << indexBits) - 1) << offsetBits;
    return address & indexMask;
}

#endif
