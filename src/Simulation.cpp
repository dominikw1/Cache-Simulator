#include "Simulation.h"
#include "CPU.h"
#include "Cache.h"
#include "InstructionCache.h"
#include "Memory.h"
#include "Policy/FIFOPolicy.h"
#include "Policy/LRUPolicy.h"
#include "Policy/Policy.h"
#include "Policy/RandomPolicy.h"
#include "Request.h"
#include "Result.h"

#include <exception>

#include <systemc>

std::unique_ptr<ReplacementPolicy<std::uint32_t>> getReplacementPolity(int replacementPolicy, int cacheSize);

template <MappingType mappingType>

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
                                      cacheLatency, std::vector<Request>(requests, requests + numRequests)};

    sc_core::sc_clock clk("Clock", sc_core::sc_time(1, sc_core::SC_NS));

    // Data Cache
    // CPU -> Cache
    sc_core::sc_signal<bool> cpuWeSignal;
    sc_core::sc_signal<std::uint32_t> cpuAddressSignal;
    sc_core::sc_signal<std::uint32_t> cpuDataOutSignal;
    sc_core::sc_signal<bool> cpuValidDataRequestSignal;

    cpu.weBus(cpuWeSignal);
    cpu.addressBus(cpuAddressSignal);
    cpu.dataOutBus(cpuDataOutSignal);
    cpu.validDataRequestBus(cpuValidDataRequestSignal);

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
    sc_core::sc_signal<std::uint32_t> ramDataInSignal;
    sc_core::sc_signal<bool> ramValidRequestSignal;

    dataRam.weBus(ramWeSignal);
    dataRam.addressBus(ramAddressSignal);
    dataRam.validRequestBus(ramValidRequestSignal);
    dataRam.dataInBus(ramAddressSignal);

    dataCache.memoryWeBus(ramWeSignal);
    dataCache.memoryAddrBus(ramAddressSignal);
    dataCache.memoryValidRequestBus(ramValidRequestSignal);
    dataCache.memoryDataOutBus(ramAddressSignal);

    // RAM -> Cache
    sc_core::sc_signal<sc_dt::sc_bv<128>> ramDataOutSignal;
    sc_core::sc_signal<bool> ramReadySignal;

    dataRam.readyBus(ramReadySignal);
    dataRam.dataOutBus(ramDataOutSignal);

    dataCache.memoryReadyBus(ramReadySignal);
    dataCache.memoryDataInBus(ramDataOutSignal);

    // Instruction Cache
    // CPU -> Cache
    sc_core::sc_signal<bool> validInstrRequestSignal;
    sc_core::sc_signal<std::uint32_t> pcSignal;

    cpu.validInstrRequestBus(validInstrRequestSignal);
    cpu.pcBus(pcSignal);

    instructionCache.validInstrRequestBus(validInstrRequestSignal);
    instructionCache.pcBus(pcSignal);

    // Cache -> CPU
    sc_core::sc_signal<Request> instructionSignal;
    sc_core::sc_signal<bool> instrReadySignal;

    cpu.instrBus(instructionSignal);
    cpu.instrReadyBus(instrReadySignal);

    instructionCache.instructionBus(instructionSignal);
    instructionCache.instrReadyBus(instrReadySignal);

    // Cache -> RAM
    sc_core::sc_signal<std::uint32_t> instrRamAddressSignal;
    sc_core::sc_signal<bool> instrRamWeBus;
    sc_core::sc_signal<bool> instrRamValidRequestBus;
    sc_core::sc_signal<std::uint32_t> instrRamDataInBus;

    instructionRam.addressBus(instrRamAddressSignal);
    instructionRam.weBus(instrRamWeBus);
    instructionRam.validRequestBus(validInstrRequestSignal);
    instructionRam.dataInBus(instrRamDataInBus);

    instructionCache.memoryAddrBus(instrRamAddressSignal);
    instructionCache.memoryWeBus(instrRamWeBus);
    instructionCache.memoryValidRequestBus(validInstrRequestSignal);
    instructionCache.memoryDataOutBus(instrRamDataInBus);

    // RAM -> Cache
    sc_core::sc_signal<sc_dt::sc_bv<128>> instrRamDataOutSignal;
    sc_core::sc_signal<bool> instrRamReadySignal;

    instructionRam.readyBus(instrRamReadySignal);
    instructionRam.dataOutBus(instrRamDataOutSignal);

    instructionCache.memoryReadyBus(instrRamReadySignal);
    instructionCache.memoryDataInBus(instrRamDataOutSignal);

    cpu.clock(clk);
    dataCache.clock(clk);
    // TODO: instructionCache.clock(clk);
    dataRam.clock(clk);
    instructionRam.clock(clk);

    // Create tracefile if option is set
    sc_core::sc_trace_file* trace;
    if (tracefile != NULL) {
        trace = sc_core::sc_create_vcd_trace_file(tracefile);

        // Data Cache signals
        sc_core::sc_trace(trace, cpuWeSignal, "cpuWeSignal");
        sc_core::sc_trace(trace, cpuAddressSignal, "cpuAddressSignal");
        sc_core::sc_trace(trace, cpuDataOutSignal, "cpuDataOutSignal");
        sc_core::sc_trace(trace, cpuValidDataRequestSignal, "cpuValidDataRequestSignal");
        sc_core::sc_trace(trace, cpuDataInSignal, "cpuDataInSignal");
        sc_core::sc_trace(trace, cpuDataReadySignal, "cpuDataReadySignal");

        sc_core::sc_trace(trace, ramWeSignal, "ramWeSignal");
        sc_core::sc_trace(trace, ramAddressSignal, "ramAddressSignal");
        sc_core::sc_trace(trace, ramDataInSignal, "ramDataInSignal");
        sc_core::sc_trace(trace, ramValidRequestSignal, "ramValidRequestSignal");
        sc_core::sc_trace(trace, ramDataOutSignal, "ramDataOutSignal");
        sc_core::sc_trace(trace, ramReadySignal, "ramReadySignal");

        // Instruction Cache signals
        sc_core::sc_trace(trace, validInstrRequestSignal, "validInstrRequestSignal");
        sc_core::sc_trace(trace, pcSignal, "pcSignal");
        sc_core::sc_trace(trace, instructionSignal, "instructionSignal");
        sc_core::sc_trace(trace, instrReadySignal, "instrReadySignal");

        sc_core::sc_trace(trace, instrRamAddressSignal, "instrRamAddressSignal");
        sc_core::sc_trace(trace, instrRamWeBus, "instrRamWeBus");
        sc_core::sc_trace(trace, instrRamValidRequestBus, "instrRamValidRequestBus");
        sc_core::sc_trace(trace, instrRamDataInBus, "instrRamDataInBus");
        sc_core::sc_trace(trace, instrRamDataOutSignal, "instrRamDataOutSignal");
        sc_core::sc_trace(trace, instrRamReadySignal, "instrRamReadySignal");
    }

    sc_core::sc_start(cycles, sc_core::SC_NS);

    if (tracefile != NULL) {
        sc_core::sc_close_vcd_trace_file(trace);
    }

    return Result{
        cpu.getElapsedCycleCount(), dataCache.missCount, dataCache.hitCount, 1 // TODO: primitiveGateCount
    };
}

struct Result run_simulation(int cycles, int directMapped, unsigned int cacheLines, unsigned int cacheLineSize,
                             unsigned int cacheLatency, unsigned int memoryLatency, size_t numRequests,
                             struct Request requests[], const char* tracefile, int policy, int usingCache) {
    if (directMapped == 0) {
        return run_simulation<MappingType::Fully_Associative>(cycles, cacheLines, cacheLineSize, cacheLatency,
                                                              memoryLatency, numRequests, requests, tracefile, policy,
                                                              usingCache);
    } else {
        return run_simulation<MappingType::Direct>(cycles, cacheLines, cacheLineSize, cacheLatency, memoryLatency,
                                                   numRequests, requests, tracefile, policy, usingCache);
    }
}

std::unique_ptr<ReplacementPolicy<std::uint32_t>> getReplacementPolity(int replacementPolicy, int cacheSize) {
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
//
int sc_main(__attribute__((unused)) int argc, __attribute__((unused)) char* argv[]) {
    std::cout << "ERROR: call to sc_main method\n";
    return 1;
}