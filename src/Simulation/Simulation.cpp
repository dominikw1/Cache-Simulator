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
    RAM dataRam{"Data_RAM", memoryLatency, cacheLineSize};
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
        sc_trace(trace, connections.get()->clk, "clock");

        // Data Cache signals
        sc_trace(trace, connections.get()->cpuWeSignal, "cpuWeSignal");
        sc_trace(trace, connections.get()->cpuAddressSignal, "cpuAddressSignal");
        sc_trace(trace, connections.get()->cpuDataOutSignal, "cpuDataOutSignal");
        sc_trace(trace, connections.get()->cpuValidDataRequestSignal, "cpuValidDataRequestSignal");
        sc_trace(trace, connections.get()->cpuDataInSignal, "cpuDataInSignal");
        sc_trace(trace, connections.get()->cpuDataReadySignal, "cpuDataReadySignal");

        sc_trace(trace, connections.get()->ramWeSignal, "ramWeSignal");
        sc_trace(trace, connections.get()->ramAddressSignal, "ramAddressSignal");
        sc_trace(trace, connections.get()->ramDataInSignal, "ramDataInSignal");
        sc_trace(trace, connections.get()->ramValidRequestSignal, "ramValidRequestSignal");
        sc_trace(trace, connections.get()->ramDataOutSignal, "ramDataOutSignal");
        sc_trace(trace, connections.get()->ramReadySignal, "ramReadySignal");

        // Instruction Cache signals
        sc_trace(trace, connections.get()->validInstrRequestSignal, "validInstrRequestSignal");
        sc_trace(trace, connections.get()->pcSignal, "pcSignal");
        sc_trace(trace, connections.get()->instructionSignal, "instructionSignal");
        sc_trace(trace, connections.get()->instrReadySignal, "instrReadySignal");

        sc_trace(trace, connections.get()->instrRamAddressSignal, "instrRamAddressSignal");
        sc_trace(trace, connections.get()->instrRamWeBus, "instrRamWeBus");
        sc_trace(trace, connections.get()->instrRamValidRequestBus, "instrRamValidRequestBus");
        sc_trace(trace, connections.get()->instrRamDataInBus, "instrRamDataInBus");
        sc_trace(trace, connections.get()->instrRamDataOutSignal, "instrRamDataOutSignal");
        sc_trace(trace, connections.get()->instrRamReadySignal, "instrRamReadySignal");
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
    std::cout << "ERROR: call to sc_main method\n";
    return 1;
}