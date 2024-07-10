#pragma once
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
    sc_core::sc_in<uint32_t> cacheInDataBus;
    sc_core::sc_in<uint32_t> cacheWeBus;

    sc_core::sc_out<uint32_t> memoryAddressBus;
    sc_core::sc_signal<uint32_t> memoryInDataBus;
    sc_core::sc_signal<uint32_t> memoryOutDataBus;
    sc_core::sc_out<int> memoryWeBus;
    sc_core::sc_in<bool> memoryReady;

    sc_core::sc_out<bool> ready;
    sc_core::sc_out<size_t> hit;
    sc_core::sc_out<size_t> miss;
    sc_core::sc_out<uint32_t> cacheOutDataBus;

private:

    size_t hitCount;
    size_t missCount;

    CacheInternal cacheInternal;
    LRUPolicy<uint32_t> lru;

    unsigned int cacheLines;
    unsigned int cacheLineSize;

    std::vector<uint32_t> tags;

    SC_CTOR(CACHE);

public:
    CACHE(sc_core::sc_module_name name, unsigned int cacheLines, unsigned int cacheLineSize)
            : sc_module{name},
              cacheInternal{cacheLines, cacheLineSize},
              lru{cacheLines},
              cacheLines{cacheLines},
              tags{cacheLines} {

        SC_THREAD(update);
        sensitive << cacheWeBus;
    }

private:
    uint32_t getTag(uint32_t address);

    uint32_t getIndex(uint32_t address);

    void update() {
        ready = false;
        if (cacheWeBus) {
            write(cacheAddressBus, cacheInDataBus);
        } else {
            cacheOutDataBus = read(cacheAddressBus);
        }
        ready = true;
    }

    uint32_t read(uint32_t address) {
        auto tag = getTag(address);
        auto index = getIndex(address);
        auto offset = getOffset(address);

        if (tags[index] != tag) {
            miss = ++missCount;

            memoryAddressBus = address;
            memoryWeBus = 0;

            sc_core::next_trigger(memoryReady.posedge_event()); // TODO: Test if this works

            tags[index] = tag;
            auto dataFromMemory = memoryOutDataBus.read();
            auto dataFromMemoryInBytes = splitUpData(dataFromMemory);
            cacheInternal.handleRAMRead(InternalRequestMemoryRead{index, dataFromMemoryInBytes});
            free(dataFromMemoryInBytes);
        } else {
            hit = ++hitCount;
        }

        return cacheInternal.handleCPURead(InternalRequestCPURead{index, offset});
    }

    void write(uint32_t address, uint32_t data) {
        auto index = getIndex(address);
        auto offset = getOffset(address);

        read(address);

        // Write to cache
        cacheInternal.handleCPUWrite(InternalRequestCPUWrite{index, offset, data});

        // Write to memory
        memoryAddressBus = address;
        memoryInDataBus = data;
        memoryWeBus = 1;
    }

    uint32_t getOffset(uint32_t address) {
        auto offsetBits = static_cast<int>(log(cacheLineSize));

        auto offsetMask = (uint32_t) ((1 << offsetBits) - 1);
        return address & offsetMask;
    }

    uint8_t* splitUpData(uint32_t data) {
        auto* bytes = static_cast<uint8_t*>(malloc(4 * sizeof(uint8_t)));

        bytes[0] = (data >> 0) & ((1 << 8) - 1);
        bytes[1] = (data >> 8) & ((1 << 8) - 1);
        bytes[2] = (data >> 16) & ((1 << 8) - 1);
        bytes[3] = (data >> 24) & ((1 << 8) - 1);

        return bytes;
    }
};

template<>
inline uint32_t CACHE<MappingType::DIRECT_MAPPED>::getTag(uint32_t address) {
    auto offsetBits = static_cast<int>(log(cacheLineSize));
    auto indexBits = static_cast<int>(log(cacheLines));

    return (address >> (offsetBits + indexBits));
}

template<>
inline uint32_t CACHE<MappingType::FULL_ASSOCIATIVE>::getTag(uint32_t address) {
    auto offsetBits = static_cast<int>(log(cacheLineSize));

    return (address >> offsetBits);
}

template<>
inline uint32_t CACHE<MappingType::FULL_ASSOCIATIVE>::getIndex(uint32_t address) {
    auto tag = getTag(address);

    // Search for tag
    auto temp = std::find(tags.begin(), tags.end(), tag);
    if (temp != tags.end()) {
        auto index = temp - tags.begin();
        lru.logUse(index);
        return index;
    }

    // Search for next empty value
    temp = std::find(tags.begin(), tags.end(), 0);
    if (temp != tags.end()) {
        auto index = temp - tags.begin();
        lru.logUse(index);
        return index;
    }

    // If cache full -> use LRU logic
    return lru.pop();
}

template<>
inline uint32_t CACHE<MappingType::DIRECT_MAPPED>::getIndex(uint32_t address) {
    auto indexBits = static_cast<int>(log(cacheLines));
    auto offsetBits = static_cast<int>(log(cacheLineSize));

    return (address >> offsetBits) & ((1 << indexBits) - 1);
}

#endif
