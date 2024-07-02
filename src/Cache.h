
#ifndef CACHE_HPP
#define CACHE_HPP

#include <systemc>

#include "Request.h"
#include "Result.h"
#include "Memory.h"
#include "CacheInternal.h"
#include "LRU.h"

enum CachePolicy {
    DIRECT_MAPPED, FULL_ASSOCIATIVE
};

template<enum CachePolicy>
SC_MODULE(CACHE) {
    sc_in<bool> clk;
    sc_in<Request> request;
    sc_out<size_t> hit;
    sc_out<size_t> miss;

    sc_signal<uint32_t> addressBus;
    sc_signal<uint32_t> dataBus;
    sc_signal<int> weBus;

    size_t hits;
    size_t misses;

    MEMORY memory;
    CacheInternal cacheInternal;
    LRU<uint32_t> lru;

    unsigned int latency;
    int directMapped;

    std::vector<uint32_t> tags;

    unsigned int cacheLines;
    unsigned int cacheLineSize;

    SC_CTOR(CACHE);

    CACHE(sc_module_name name, int directMapped, unsigned int cacheLines, unsigned int cacheLineSize,
          unsigned int cacheLatency, unsigned int memoryLatency)
            : sc_module{name},
              memory{"memory", memoryLatency},
              cacheInternal(cacheLines, cacheLineSize),
              latency{cacheLatency},
              directMapped{directMapped},
              cacheLines{cacheLines},
              tags{cacheLines} {

        memory.weBus(weBus);
        memory.dataBus(dataBus);
        memory.addressBus(addressBus);

        SC_THREAD(update);
        sensitive << request;
    }

    void update() {
        //::wait(latency);

        if (request.read().we) {
            read(request);
        } else {
            write(request);
        }
    }

    uint32_t read(Request request) {
        auto tag = getTag(request.addr);
        auto index = getIndex(request.addr);
        auto offset = getOffset(request.addr);

        if (tags[index] != tag) {
            weBus = 0;
            addressBus = request.addr;

            tags[index] = tag;

            auto dataFromMemory = dataBus.read();
            cacheInternal.handleRAMRead(InternalRequestMemoryRead{index, splitUpData(dataFromMemory)});

            misses = ++misses;
        } else {
            hit = ++hits;
        }

        return cacheInternal.handleCPURead(InternalRequestCPURead{index, offset});
    }

    void write(Request request) {
        auto tag = getTag(request.addr);
        auto index = getIndex(request.addr);
        auto offset = getOffset(request.addr);

        // Write to cache
        cacheInternal.handleCPUWrite(InternalRequestCPUWrite{index, offset, request.data});

        tags[index] = tag;

        // Write to memory
        addressBus = request.addr;
        dataBus = request.data;
        weBus = 1;
    }

    uint32_t getTag(uint32_t address) {
        switch (directMapped) {
            case 0: {
                int offsetBits = log(cacheLineSize);

                int tagMasks = (-1 << offsetBits);
                return address & tagMasks;
            }
            default: {
                int offsetBits = log(cacheLineSize);
                int indexBits = log(cacheLines);

                int tagMasks = (-1 << (indexBits + offsetBits));
                return address & tagMasks;
            }
        }
    }

    uint32_t getIndex(uint32_t address) {
        switch (directMapped) {
            case 0: {
                int tag = getTag(address);
                auto temp = std::find(tags.begin(), tags.end(), tag);

                if (temp != tags.end()) {
                    return temp - tags.begin();
                }

                temp = std::find(tags.begin(), tags.end(), 0);

                if (temp != tags.end()) {
                    return temp - tags.begin();
                }

                return lru.popLRU();
            }
            default: {
                int indexBits = log(cacheLines);
                int offsetBits = log(cacheLineSize);

                int indexMask = ((1 << indexBits) - 1) << offsetBits;
                return address & indexMask;
            }
        }

    }

    uint32_t getOffset(uint32_t address) {
        int offsetBits = log(cacheLineSize);
        int offsetMask = (1 << offsetBits) - 1;

        return address & offsetMask;
    }

    uint8_t* splitUpData(uint32_t data) {
        uint8_t bytes[4];

        bytes[0] = (data >> 0) & ((1 << 8) - 1);
        bytes[1] = (data >> 8) & ((1 << 8) - 1);
        bytes[2] = (data >> 16) & ((1 << 8) - 1);
        bytes[3] = (data >> 24) & ((1 << 8) - 1);

        return bytes;
    }
};

#endif
