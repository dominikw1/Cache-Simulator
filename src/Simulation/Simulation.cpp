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
#include <memory>

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
    if (tracefile != nullptr) {
        auto traceCloser = [](sc_core::sc_trace_file* trace) { sc_close_vcd_trace_file(trace); };
        std::unique_ptr<sc_core::sc_trace_file, decltype(traceCloser)> trace{sc_create_vcd_trace_file(tracefile),
                                                                             std::move(traceCloser)};
        trace->set_time_unit(100, SC_PS); // not 1 NS because write buffer does some things at falling edge
        sc_trace(trace.get(), connections.get()->clk, "clock");

        // Data Cache signals
        sc_trace(trace.get(), connections.get()->CPU_to_dataCache_Address, "CPU_to_dataCache_Address");
        sc_trace(trace.get(), connections.get()->CPU_to_dataCache_Data, "CPU_to_dataCache_Data");
        sc_trace(trace.get(), connections.get()->CPU_to_dataCache_WE, "CPU_to_dataCache_WE");
        sc_trace(trace.get(), connections.get()->CPU_to_dataCache_Vaid_Request, "CPU_to_dataCache_Vaid_Request");
        sc_trace(trace.get(), connections.get()->dataCache_to_CPU_Data, "dataCache_to_CPU_Data");
        sc_trace(trace.get(), connections.get()->dataCache_to_CPU_Ready, "dataCache_to_CPU_Ready");

        sc_trace(trace.get(), connections.get()->dataCache_to_dataRAM_Address, "dataCache_to_dataRAM_Address");
        sc_trace(trace.get(), connections.get()->dataCache_to_dataRAM_Data, "dataCache_to_dataRAM_Data");
        sc_trace(trace.get(), connections.get()->dataCache_to_dataRAM_WE, "dataCache_to_dataRAM_WE");
        sc_trace(trace.get(), connections.get()->dataCache_to_dataRAM_Valid_Request, "dataCache_to_dataRAM_Valid_Request");
        sc_trace(trace.get(), connections.get()->dataRAM_to_dataCache_Data, "dataRAM_to_dataCache_Data");
        sc_trace(trace.get(), connections.get()->dataRAM_to_dataCache_Ready, "dataRAM_to_dataCache_Ready");

        // Instruction Cache signals
        sc_trace(trace.get(), connections.get()->CPU_to_instrCache_PC, "CPU_to_instrCache_PC");
        sc_trace(trace.get(), connections.get()->CPU_to_instrCache_Valid_Request, "CPU_to_instrCache_Valid_Request");
        sc_trace(trace.get(), connections.get()->instrCache_to_CPU_Instruction, "instrCache_to_CPU_Instruction");
        sc_trace(trace.get(), connections.get()->instrCache_to_CPU_Ready, "instrCache_to_CPU_Ready");

        sc_trace(trace.get(), connections.get()->instrCache_to_instrRAM_Address, "instrCache_to_instrRAM_Address");
        sc_trace(trace.get(), connections.get()->instrCache_to_instrRAM_Data, "instrCache_to_instrRAM_Data");
        sc_trace(trace.get(), connections.get()->instrCache_to_instrRAM_WE, "instrCache_to_instrRAM_WE");
        sc_trace(trace.get(), connections.get()->instrCache_to_instrRAM_Valid_Request,
                 "instrCache_to_instrRAM_Valid_Request");
        sc_trace(trace.get(), connections.get()->instrRAM_to_instrCache_Data, "instrRAM_to_instrCache_Data");
        sc_trace(trace.get(), connections.get()->instrRAM_to_instrCache_Ready, "instrRAM_to_instrCache_Ready");
        dataCache.traceInternalSignals(trace.get());
    }

    sc_start(sc_time::from_value(cycles * 1000ull)); // from_value takes pico-seconds and each of our cycles is a NS


    return Result{
        connections.get()->CPU_to_instrCache_PC >= numRequests - 1 ? cpu.getElapsedCycleCount() : SIZE_MAX,
        dataCache.missCount, dataCache.hitCount,
        dataCache.calculateGateCount() // TODO: primitiveGateCount
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