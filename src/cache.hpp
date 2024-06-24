
#ifndef CACHE_HPP
#define CACHE_HPP

#include "systemc.h"
#include <systemc>

#include "Request.h"
#include "Result.h"
#include "memory.hpp"

using namespace sc_core;

SC_MODULE(CACHE) {
    sc_in<bool> clk;
    // sc_in<Request> request;
    sc_out<size_t> hit;
    sc_out<size_t> miss;

    sc_signal<uint32_t> addressBus;
    sc_signal<uint32_t> dataBus;
    sc_signal<int> weBus;

    unsigned* cache; // TODO: Welcher typ?
    int latency;
    int directMapped;

    SC_CTOR(CACHE);

    CACHE(sc_module_name name, int directMapped, unsigned cacheLines, unsigned cacheLineSize, unsigned int cacheLatency,
          unsigned int memoryLatency)
        : sc_module{name} {
        this->latency = cacheLatency;
        this->directMapped = directMapped;

        // memory.weBus(weBus);
        // memory.dataBus(dataBus);
        // memory.addressBus(addressBus);

        SC_THREAD(update);
        sensitive << clk.pos(); // TODO: Or request?
    }

    void update() {
        wait(latency);

        // TODO: Mit Templates implementieren
        // -> isFull()
        // -> getCacheLine()
        if (directMapped) {
            // TODO: Immer DataBus setzen oder nur wenn we = 1?
        } else {
        }
    }
};

#endif
