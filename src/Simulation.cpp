#include "Simulation.h"
#include "CPU.h"
#include "Cache.h"
// #include "InstructionCache.h"
#include "FIFOPolicy.h"
#include "InstructionCache.h"
#include "LRUPolicy.h"
#include "Memory.h"
#include "Policy.h"
#include "RandomPolicy.h"
#include "Request.h"
#include "Result.h"

#include <exception>

#include <systemc>

std::unique_ptr <ReplacementPolicy<std::uint32_t>> getReplacementPolity(int replacementPolicy, int cacheSize);

template<MappingType mappingType>
Result run_simulation(int cycles, unsigned int cacheLines, unsigned int cacheLineSize, unsigned int cacheLatency,
                      unsigned int memoryLatency, size_t numRequests, struct Request requests[], const char* tracefile,
                      int policy, int usingCache) {


    std::cout << "Starting Simulation...\n";

    const unsigned int instructionCacheLineSize = 64;
    const unsigned int instructionCacheLines = 10;

    CPU cpu{"CPU"};
    RAM dataRam{"Data_RAM", cacheLineSize, memoryLatency};
    RAM instructionRam{"Instruction_RAM", instructionCacheLineSize, memoryLatency};
    Cache<mappingType> dataCache{"Data_cache", cacheLines, cacheLineSize, cacheLatency,
                                 getReplacementPolity(policy, cacheLines)};
    InstructionCache instructionCache{"Instruction_Cache", instructionCacheLines, instructionCacheLineSize,
                                      std::vector<Request>(requests, requests + numRequests)};

    sc_core::sc_clock clk("Clock", sc_core::sc_time(1, sc_core::SC_SEC));

    // Data Cache
    // CPU -> Cache
    sc_core::sc_signal<bool> cpuWeSignal;
    sc_core::sc_signal <std::uint32_t> cpuAddressSignal;
    sc_core::sc_signal <std::uint32_t> cpuDataOutSignal;
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
    sc_core::sc_signal <std::uint32_t> cpuDataInSignal;
    sc_core::sc_signal<bool> cpuDataReadySignal;

    cpu.dataInBus(cpuDataInSignal);
    cpu.dataReadyBus(cpuDataReadySignal);

    dataCache.cpuDataInBus(cpuDataInSignal);
    dataCache.ready(cpuDataReadySignal);

    // Cache -> RAM
    sc_core::sc_signal<bool> ramWeSignal;
    sc_core::sc_signal <std::uint32_t> ramAddressSignal;
    std::vector <sc_core::sc_signal<std::uint8_t>> ramDataInSignals{cacheLineSize};
    sc_core::sc_signal<bool> ramValidDataRequestSignal;

    dataRam.weBus(ramWeSignal);
    dataRam.addressBus(ramAddressSignal);
    dataRam.validRequest(ramValidDataRequestSignal);

    dataCache.memoryWeBus(ramWeSignal);
    dataCache.memoryAddrBus(ramAddressSignal);
    dataCache.memoryValidRequestBus(ramValidDataRequestSignal);

    for (int i = 0; i < cacheLineSize; ++i) {
        dataRam.dataInBusses[i](ramDataInSignals[i]);
        dataCache.memoryDataOutBusses[i](ramDataInSignals[i]);
    }

    // RAM -> Cache
    std::vector <sc_core::sc_signal<std::uint8_t>> ramDataOutSignals{cacheLineSize};
    sc_core::sc_signal<bool> ramReadySignal;

    dataRam.readyBus(ramReadySignal);

    dataCache.memoryReadyBus(ramReadySignal);

    for (int i = 0; i < cacheLineSize; ++i) {
        dataRam.dataOutBusses[i](ramDataOutSignals[i]);
        dataCache.memoryDataInBusses[i](ramDataOutSignals[i]);
    }

    // Instruction Cache
    // CPU -> Cache
    sc_core::sc_signal<bool> validInstrRequestSignal;
    sc_core::sc_signal <std::uint32_t> pcSignal;

    cpu.validInstrRequestBus(validInstrRequestSignal);
    cpu.pcBus(pcSignal);

    instructionCache.validInstrRequestBus(validInstrRequestSignal);
    instructionCache.pcBus(pcSignal);

    // Cache -> CPU
    sc_core::sc_signal <Request> instructionSignal;
    sc_core::sc_signal<bool> instrReadySignal;

    cpu.instrBus(instructionSignal);
    cpu.instrReadyBus(instrReadySignal);

    instructionCache.instructionBus(instructionSignal);
    instructionCache.instrReadyBus(instrReadySignal);

    // Cache -> RAM
    sc_core::sc_signal <std::uint32_t> instrRamAddressSignal;
    sc_core::sc_signal<bool> instrRamWeBus;
    sc_core::sc_signal<bool> instrRamValidRequestBus;
    std::vector <sc_core::sc_signal<std::uint8_t>> instrRamDataInBusses(cacheLineSize);

    instructionRam.addressBus(instrRamAddressSignal);
    instructionRam.weBus(instrRamWeBus);
    instructionRam.validRequest(validInstrRequestSignal);

    instructionCache.memoryAddrBus(instrRamAddressSignal);
    instructionCache.memoryWeBus(instrRamWeBus);
    instructionCache.memoryValidRequestBus(validInstrRequestSignal);

    for (int i = 0; i < cacheLineSize; ++i) {
        instructionRam.dataInBusses[i](instrRamDataInBusses[i]);
        instructionCache.memoryDataOutBusses[i](instrRamDataInBusses[i]);
    }

    // RAM -> Cache
    std::vector <sc_core::sc_signal<std::uint8_t>> instrRamDataOutSignal;
    sc_core::sc_signal<bool> instrRamReadySignal;

    instructionRam.readyBus(instrRamReadySignal);

    instructionCache.memoryReadyBus(instrRamReadySignal);

    for (int i = 0; i < cacheLineSize; ++i) {
        instructionRam.dataOutBusses[i](instrRamDataOutSignal[i]);
        instructionCache.memoryDataInBusses[i](instrRamDataOutSignal[i]);
    }

    cpu.clock(clk);

    sc_core::sc_trace_file* file = sc_core::sc_create_vcd_trace_file(tracefile);
    // TODO: Add all signals

    sc_core::sc_start(cycles, sc_core::SC_SEC);

    sc_core::sc_close_vcd_trace_file(file);

    return Result{
            cpu.getElapsedCycleCount(), dataCache.missCount, dataCache.hitCount, 1 // TODO: primitiveGateCount
    };
}

struct Result run_simulation(int cycles, int directMapped, unsigned int cacheLines, unsigned int cacheLineSize,
                             unsigned int cacheLatency, unsigned int memoryLatency, size_t numRequests,
                             struct Request requests[], const char* tracefile, int policy, int usingCache) {
    if (directMapped == 0) {
        return run_simulation<MappingType::Fully_Associative>(cycles, cacheLines, cacheLineSize, cacheLatency,
                                                              memoryLatency,
                                                              numRequests, requests, tracefile, policy, usingCache);
    } else {
        return run_simulation<MappingType::Direct>(cycles, cacheLines, cacheLineSize, cacheLatency,
                                                   memoryLatency, numRequests, requests, tracefile, policy,
                                                   usingCache);
    }
}

std::unique_ptr <ReplacementPolicy<std::uint32_t>> getReplacementPolity(int replacementPolicy, int cacheSize) {
    switch (replacementPolicy) {
        case 0:
            return std::make_unique<LRUPolicy<std::uint32_t>>(cacheSize);
        case 1:
            return std::make_unique<FIFOPolicy<std::uint32_t>>(cacheSize);
        case 2:
            return std::make_unique<RandomPolicy<std::uint32_t>>(cacheSize);
        default:
            throw std::runtime_error("Encountered unknown policy type");
    }
}

using namespace sc_core;

int sc_main(int argc, char* argv[]) {
    std::cout << "ERROR: call to sc_main method" << std::endl;
    return 1;
}