#include "Simulation.h"
#include "CPU.h"
#include "Cache.h"
//#include "InstructionCache.h"
#include "Request.h"
#include "Result.h"
#include "Policy.h"
#include "Memory.h"
#include "LRUPolicy.h"
#include "FIFOPolicy.h"
#include "RandomPolicy.h"
#include "InstructionCache.h"

#include <systemc>

std::unique_ptr<ReplacementPolicy<std::uint32_t>> getReplacementPolity(int replacementPolicy, int cacheSize);

struct Result run_simulation(int cycles, int directMapped, unsigned int cacheLines, unsigned int cacheLineSize,
                             unsigned int cacheLatency, unsigned int memoryLatency, size_t numRequests,
                             struct Request requests[], const char* tracefile, int policy, int usingCache) {
    std::cout << "Starting Simulation...\n";

    CPU cpu{"CPU"}; // TODO: How does to latency work in the CPU
    RAM ram{"RAM", cacheLineSize, memoryLatency};
    Cache<MappingType::Direct> dataCache{"Data cache", cacheLines, cacheLineSize, cacheLatency,
                                         getReplacementPolity(policy, cacheLines)};
    //InstructionCache{"Instruction Cache"};

    sc_core::sc_clock clk;

    // Data Cache
    // CPU -> Cache
    sc_core::sc_signal<bool> cpuWeSignal;
    sc_core::sc_signal<std::uint32_t> cpuAddressSignal;
    sc_core::sc_signal<std::uint32_t> cpuDataOutSignal;
    sc_core::sc_signal<bool> cpuValidDataRequestSignal;

    cpu.weBus(cpuWeSignal);
    cpu.addressBus(cpuAddressSignal);
    cpu.dataOutBus(cpuDataOutSignal);
    cpu.validDataRequest(cpuValidDataRequestSignal);

    dataCache.cpuWeBus(cpuWeSignal);
    dataCache.cpuAddrBus(cpuAddressSignal);
    dataCache.cpuDataOutBus(cpuDataOutSignal);
    dataCache.cpuValidRequest(cpuValidDataRequestSignal);

    // Cache -> CPU
    sc_core::sc_signal<std::uint32_t> cpuDataInSignal;
    sc_core::sc_signal<bool> cpuDataReadySignal;

    cpu.dataInBus(cpuDataInSignal);
    cpu.dataReadyBus(cpuDataReadySignal);

    dataCache.cpuDataInBus(cpuDataInSignal);
    dataCache.ready(cpuDataReadySignal);

    // Cache -> RAM
    sc_core::sc_signal<bool> ramWeSignal;
    sc_core::sc_signal<std::uint32_t> ramAddressSignal;
    std::vector<sc_core::sc_signal<std::uint8_t>> ramDataInSignals{cacheLineSize};
    sc_core::sc_signal<bool> ramValidDataRequestSignal;

    ram.weBus(ramWeSignal);
    ram.addressBus(ramAddressSignal);
    ram.validDataRequest(ramValidDataRequestSignal);

    dataCache.memoryWeBus(ramWeSignal);
    dataCache.memoryAddrBus(ramAddressSignal);
    dataCache.memoryValidRequestBus(ramValidDataRequestSignal);

    for (int i = 0; i < cacheLineSize; ++i) {
        ram.dataInBusses[i](ramDataInSignals[i]);
        dataCache.memoryDataOutBusses[i](ramDataInSignals[i]);
    }

    // RAM -> Cache
    std::vector<sc_core::sc_signal<std::uint8_t>> ramDataOutSignals{cacheLineSize};
    sc_core::sc_signal<bool> ramReadySignal;

    ram.readyBus(ramReadySignal);

    dataCache.memoryReadyBus(ramReadySignal);

    for (int i = 0; i < cacheLineSize; ++i) {
        ram.dataOutBusses[i](ramDataOutSignals[i]);
        dataCache.memoryDataInBusses[i](ramDataOutSignals[i]);
    }

    cpu.clock(clk);

    sc_core::sc_start();

    return Result{
            0, dataCache.missCount, dataCache.hitCount, 1 // TODO: primitiveGateCount
    };
}


std::unique_ptr<ReplacementPolicy<std::uint32_t>> getReplacementPolity(int replacementPolicy, int cacheSize) {
    switch (replacementPolicy) {
        case 0:
            return std::make_unique<LRUPolicy<std::uint32_t>>(cacheSize);
        case 1:
            return std::make_unique<FIFOPolicy<std::uint32_t>>(cacheSize);
        case 2:
            return std::make_unique<RandomPolicy<std::uint32_t>>(cacheSize);
    }
}

using namespace sc_core;

int sc_main(int argc, char* argv[]) {
    std::cout << "ERROR: call to sc_main method" << std::endl;
    return 1;
}