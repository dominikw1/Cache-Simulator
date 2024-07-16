#pragma once

#include "Cache.h"
#include "Request.h"

#include "Policy/FIFOPolicy.h"
#include <memory>
#include <systemc>

// optimsiation: instruction buffer
SC_MODULE(InstructionCache) {
private:
    unsigned int cacheLineNum;
    unsigned int cacheLineSize;
    unsigned int cacheLatency;

    Cache<MappingType::Direct> cache{"Cache", cacheLineNum, cacheLineSize, cacheLatency,
                                     std::make_unique<FIFOPolicy<std::uint32_t>>(cacheLineNum)};

    std::vector<Request> instructions;

    SC_CTOR(InstructionCache);

public:
    sc_core::sc_in<bool> clock;

    // -> CPU
    sc_core::sc_out<Request> instructionBus;
    sc_core::sc_out<bool> instrReadyBus;

    // <- CPU
    sc_core::sc_in<bool> validInstrRequestBus;
    sc_core::sc_in<std::uint32_t> pcBus;

    // Cache -> RAM
    sc_core::sc_out<std::uint32_t> memoryAddrBus{"memoryAddrBus"};
    sc_core::sc_out<std::uint32_t> memoryDataOutBus{"memoryDataOutBus"};
    sc_core::sc_out<bool> memoryWeBus{"memoryWeBus"};
    sc_core::sc_out<bool> memoryValidRequestBus{"memoryValidRequestBus"};

    // RAM -> Cache
    sc_core::sc_in<sc_dt::sc_bv<128>> memoryDataInBus;
    sc_core::sc_in<bool> memoryReadyBus{"memoryReadyBus"};

    InstructionCache(sc_core::sc_module_name name, unsigned int cacheLines, unsigned int cacheLineSize,
                     unsigned int cacheLatency, std::vector<Request> instructions) : sc_module{name},
                                                                                     cacheLineNum{cacheLines},
                                                                                     cacheLineSize{cacheLineSize},
                                                                                     cacheLatency{cacheLatency},
                                                                                     instructions{instructions} {
        SC_THREAD(provideInstruction);
        sensitive << clock.pos();
    }

    void provideInstruction() {
        while (true) {
            wait(clock.posedge_event());
            instrReadyBus.write(false);

            do {
                wait(clock.posedge_event());
            } while (!validInstrRequestBus);

            auto pc = pcBus.read();
            if (pc >= instructions.size()) {
                sc_core::sc_stop();
                return;
            }

            // Act like we are fetching data from Memory
            cache.memoryAddrBus.write(pcBus.read());
            cache.memoryWeBus.write(false);
            cache.memoryValidRequestBus.write(true);

            do {
                wait(clock.posedge_event());
            } while (!cache.memoryReadyBus);

            instructionBus.write(instructions[pc]);
            instrReadyBus.write(true);
        }
    }
};