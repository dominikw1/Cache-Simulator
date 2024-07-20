#include "Simulation.h"
#include "CPU.h"
#include "Cache.h"
#include "Connections.h"
#include "InstructionCache.h"
#include "Memory.h"
#include "Policy/FIFOPolicy.h"
#include "Policy/LRUPolicy.h"
#include "Policy/Policy.h"
#include "Policy/RandomPolicy.h"

#include <exception>

#include <systemc>

using namespace sc_core;

std::unique_ptr<ReplacementPolicy<std::uint32_t>> getReplacementPolity(CacheReplacementPolicy replacementPolicy,
                                                                       int cacheSize);

constexpr std::uint8_t instructionCacheLineSize = 64;
constexpr std::uint8_t instructionCacheNumLines = 16;

template <MappingType mappingType>
Result run_simulation_extended(unsigned int cycles, unsigned int cacheLines, unsigned int cacheLineSize,
                               unsigned int cacheLatency, unsigned int memoryLatency, size_t numRequests,
                               struct Request requests[], const char* tracefile, CacheReplacementPolicy policy,
                               int usingCache) {
    std::cout << "Starting Simulation...\n";

    CPU cpu{"CPU", requests, numRequests};
    RAM dataRam{"Data_RAM", memoryLatency, cacheLineSize / 16};
    RAM instructionRam{"Instruction_RAM", memoryLatency, instructionCacheLineSize};

    Cache<mappingType> dataCache{"Data_cache", cacheLines, cacheLineSize, cacheLatency,
                                 (mappingType == MappingType::Direct) ? nullptr
                                                                      : getReplacementPolity(policy, cacheLines)};

    InstructionCache instructionCache{"Instruction_Cache", instructionCacheNumLines, instructionCacheLineSize,
                                      cacheLatency, std::vector<Request>(requests, requests + numRequests)};
#ifdef STRICT_INSTRUCTION_ORDER
    dataCache.setMemoryLatency(memoryLatency);
    instructionCache.setMemoryLatency(memoryLatency);
#endif

    auto connections = connectComponents(cpu, dataRam, instructionRam, dataCache, instructionCache);

    // Create tracefile if option is set
    sc_trace_file* trace;
    if (tracefile != NULL) {
        trace = sc_create_vcd_trace_file(tracefile);
        trace->set_time_unit(100, SC_PS);
        sc_trace(trace, connections.get()->clk, "clock");

        // Data Cache signals
        sc_trace(trace, connections.get()->cpuWeSignal, "CPU_Cache_WE");
        sc_trace(trace, connections.get()->cpuAddressSignal, "CPU_Cache_Address");
        sc_trace(trace, connections.get()->cpuDataOutSignal, "CPU_Cache_Data_Out");
        sc_trace(trace, connections.get()->cpuValidDataRequestSignal, "CPU_Cache_Valid_Request");
        sc_trace(trace, connections.get()->cpuDataInSignal, "Cache_CPU_Data_In");
        sc_trace(trace, connections.get()->cpuDataReadySignal, "Cache_CPU_Ready");

        sc_trace(trace, connections.get()->ramWeSignal, "Cache_RAM_WE");
        sc_trace(trace, connections.get()->ramAddressSignal, "Cache_RAM_Address");
        sc_trace(trace, connections.get()->ramDataInSignal, "Cache_RAM_Data_In");
        sc_trace(trace, connections.get()->ramValidRequestSignal, "Cache_RAM_Valid_Request");
        sc_trace(trace, connections.get()->ramDataOutSignal, "RAM_Cache_Data_Out");
        sc_trace(trace, connections.get()->ramReadySignal, "RAM_Cache_Ready");

        // Instruction Cache signals
        sc_trace(trace, connections.get()->validInstrRequestSignal, "Instr_CPU_Cache_Valid_Request");
        sc_trace(trace, connections.get()->pcSignal, "Instr_CPU_Cache_PC");
        sc_trace(trace, connections.get()->instructionSignal, "Instr_Cache_CPU_Instruction");
        sc_trace(trace, connections.get()->instrReadySignal, "Instr_Cache_CPU_Ready");

        sc_trace(trace, connections.get()->instrRamAddressSignal, "Instr_Cache_RAM_Address");
        sc_trace(trace, connections.get()->instrRamWeBus, "Instr_Cache_RAM_WE");
        sc_trace(trace, connections.get()->instrRamValidRequestBus, "Instr_Cache_RAM_Valid_Request");
        sc_trace(trace, connections.get()->instrRamDataInBus, "Instr_Cache_RAM_Data_In");
        sc_trace(trace, connections.get()->instrRamDataOutSignal, "Instr_RAM_Cache_Data_out");
        sc_trace(trace, connections.get()->instrRamReadySignal, "Instr_RAM_Cache_Ready");
        dataCache.traceInternalSignals(trace);
    }

    sc_start(sc_time::from_value(cycles * 1000ull)); // from_value takes pico-seconds and each of our cycles is a NS

    if (tracefile != NULL) {
        sc_close_vcd_trace_file(trace);
    }

    return Result{
        connections.get()->pcSignal >= numRequests - 1 ? cpu.getElapsedCycleCount() : SIZE_MAX, dataCache.missCount,
        dataCache.hitCount,
        1 // TODO: primitiveGateCount
    };
}

struct Result run_simulation_extended(unsigned int cycles, int directMapped, unsigned int cacheLines,
                                      unsigned int cacheLineSize, unsigned int cacheLatency, unsigned int memoryLatency,
                                      size_t numRequests, struct Request requests[], const char* tracefile,
                                      CacheReplacementPolicy policy, int usingCache) {
    if (directMapped == 0) {
        return run_simulation_extended<MappingType::Fully_Associative>(cycles, cacheLines, cacheLineSize, cacheLatency,
                                                                       memoryLatency, numRequests, requests, tracefile,
                                                                       policy, usingCache);
    } else {
        return run_simulation_extended<MappingType::Direct>(cycles, cacheLines, cacheLineSize, cacheLatency,
                                                            memoryLatency, numRequests, requests, tracefile, policy,
                                                            usingCache);
    }
}

struct Result run_simulation(int cycles, int directMapped, unsigned cacheLines, unsigned cacheLineSize,
                             unsigned cacheLatency, unsigned memoryLatency, size_t numRequests,
                             struct Request requests[], const char* tracefile) {
    return run_simulation_extended(cycles, directMapped, cacheLines, cacheLineSize, cacheLatency, memoryLatency,
                                   numRequests, requests, tracefile, POLICY_LRU, true);
}

std::unique_ptr<ReplacementPolicy<std::uint32_t>> getReplacementPolity(CacheReplacementPolicy replacementPolicy,
                                                                       int cacheSize) {
    switch (replacementPolicy) {
    case POLICY_LRU:
        return std::make_unique<LRUPolicy<std::uint32_t>>(cacheSize);
    case POLICY_FIFO:
        return std::make_unique<FIFOPolicy<std::uint32_t>>(cacheSize);
    case POLICY_RANDOM:
        return std::make_unique<RandomPolicy<std::uint32_t>>(cacheSize);
    default:
        throw std::runtime_error("Encountered unknown policy type");
    }
}

int sc_main(__attribute__((unused)) int argc, __attribute__((unused)) char* argv[]) {
    std::cout << "ERROR: Call to sc_main method!\n";
    return 1;
}