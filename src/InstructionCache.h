#pragma once

#include "Cache.h"
#include "Request.h"

#include "RandomPolicy.h"
#include <memory>
#include <systemc>

// optimsiation: instruction buffer
SC_MODULE(InstructionCache) {
private:
    unsigned int cacheLineNum;
    unsigned int cacheLineSize;
    Cache<MappingType::Direct> cache{"Cache", cacheLineNum, cacheLineSize, 10,
                                     std::make_unique<RandomPolicy<std::uint32_t>>(cacheLineNum)};

    std::vector<Request> instructions;

    SC_CTOR(InstructionCache);

    void decode();

    void fetch();

public:
    InstructionCache(sc_core::sc_module_name name, unsigned int cacheLines, unsigned int cacheLineSize,
                     std::vector<Request> instructions);

    // -> CPU
    sc_core::sc_out<Request> instructionBus;
    sc_core::sc_out<bool> instrReadyBus;

    // <- CPU
    sc_core::sc_in<bool> validInstrRequestBus;
    sc_core::sc_in<std::uint32_t> pcBus;

    // Cache -> RAM
    sc_core::sc_out<std::uint32_t> memoryAddrBus{"memoryAddrBus"};
    std::vector<sc_core::sc_out<std::uint8_t>> memoryDataOutBusses;
    sc_core::sc_out<bool> memoryWeBus{"memoryWeBus"};
    sc_core::sc_out<bool> memoryValidRequestBus{"memoryValidRequestBus"};

    // RAM -> Cache
    std::vector<sc_core::sc_in<std::uint8_t>> memoryDataInBusses;
    sc_core::sc_in<bool> memoryReadyBus{"memoryReadyBus"};
};