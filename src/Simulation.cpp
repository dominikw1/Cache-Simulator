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

using namespace sc_core;

std::unique_ptr<ReplacementPolicy<std::uint32_t>> getReplacementPolity(int replacementPolicy, int cacheSize);

constexpr std::uint8_t instructionCacheLineSize = 64;
constexpr std::uint8_t instructionCacheNumLines = 16;

template <MappingType mappingType>
Result run_simulation(int cycles, unsigned int cacheLines, unsigned int cacheLineSize, unsigned int cacheLatency,
                      unsigned int memoryLatency, size_t numRequests, struct Request requests[], const char* tracefile,
                      int policy, int usingCache) {
    std::cout << "Starting Simulation...\n";

    std::cout<<cacheLatency<<" "<<memoryLatency<<std::endl;

    CPU cpu{"CPU"};
    RAM dataRam{"Data_RAM", cacheLineSize, memoryLatency};
    RAM instructionRam{"Instruction_RAM", instructionCacheLineSize, memoryLatency};
    Cache<mappingType> dataCache{"Data_cache", cacheLines, cacheLineSize, cacheLatency,
                                 (mappingType == MappingType::Direct) ? nullptr
                                                                      : getReplacementPolity(policy, cacheLines)};
    InstructionCache instructionCache{"Instruction_Cache", instructionCacheNumLines, instructionCacheLineSize,
                                      cacheLatency, std::vector<Request>(requests, requests + numRequests)};

    sc_clock clk("Clock", sc_time(1, SC_NS));

    // Data Cache
    // CPU -> Cache
    sc_signal<bool> cpuWeSignal("cpuWeSignal");
    sc_signal<std::uint32_t> cpuAddressSignal("cpuAddressSignal");
    sc_signal<std::uint32_t> cpuDataOutSignal;
    sc_signal<bool> cpuValidDataRequestSignal("cpuValidDataRequestSignal");

    cpu.weBus(cpuWeSignal);
    cpu.addressBus(cpuAddressSignal);
    cpu.dataOutBus(cpuDataOutSignal);
    cpu.validDataRequestBus(cpuValidDataRequestSignal);

    dataCache.cpuWeBus(cpuWeSignal);
    dataCache.cpuAddrBus(cpuAddressSignal);
    dataCache.cpuDataInBus(cpuDataOutSignal);
    dataCache.cpuValidRequest(cpuValidDataRequestSignal);

    // Cache -> CPU
    sc_signal<std::uint32_t> cpuDataInSignal("cpuDataInSignal");
    sc_signal<bool> cpuDataReadySignal("cpuDataReadySignal");

    cpu.dataInBus(cpuDataInSignal);
    cpu.dataReadyBus(cpuDataReadySignal);

    dataCache.cpuDataOutBus(cpuDataInSignal);
    dataCache.ready(cpuDataReadySignal);

    // Cache -> RAM
    sc_signal<bool, SC_MANY_WRITERS> ramWeSignal("ramWeSignal");
    sc_signal<std::uint32_t, SC_MANY_WRITERS> ramAddressSignal("ramAddressSignal");
    sc_signal<std::uint32_t> ramDataInSignal("ramDataInSignal");
    sc_signal<bool, SC_MANY_WRITERS> ramValidRequestSignal;

    dataRam.weBus(ramWeSignal);
    dataRam.addressBus(ramAddressSignal);
    dataRam.validRequestBus(ramValidRequestSignal);
    dataRam.dataInBus(ramDataInSignal);

    dataCache.memoryWeBus(ramWeSignal);
    dataCache.memoryAddrBus(ramAddressSignal);
    dataCache.memoryValidRequestBus(ramValidRequestSignal);
    dataCache.memoryDataOutBus(ramDataInSignal);

    // RAM -> Cache
    sc_signal<sc_dt::sc_bv<128>> ramDataOutSignal;
    sc_signal<bool> ramReadySignal;

    dataRam.readyBus(ramReadySignal);
    dataRam.dataOutBus(ramDataOutSignal);

    dataCache.memoryReadyBus(ramReadySignal);
    dataCache.memoryDataInBus(ramDataOutSignal);

    // Instruction Cache
    // CPU -> Cache
    sc_signal<bool> validInstrRequestSignal("validInstrRequestSignal");
    sc_signal<std::uint32_t> pcSignal;

    cpu.validInstrRequestBus(validInstrRequestSignal);
    cpu.pcBus(pcSignal);

    instructionCache.validInstrRequestBus(validInstrRequestSignal);
    instructionCache.pcBus(pcSignal);

    // Cache -> CPU
    sc_signal<Request> instructionSignal;
    sc_signal<bool> instrReadySignal;

    cpu.instrBus(instructionSignal);
    cpu.instrReadyBus(instrReadySignal);

    instructionCache.instructionBus(instructionSignal);
    instructionCache.instrReadyBus(instrReadySignal);

    // Cache -> RAM
    sc_signal<std::uint32_t> instrRamAddressSignal;
    sc_signal<bool> instrRamWeBus;
    sc_signal<bool> instrRamValidRequestBus;
    sc_signal<std::uint32_t> instrRamDataInBus;

    instructionRam.addressBus(instrRamAddressSignal);
    instructionRam.weBus(instrRamWeBus);
    instructionRam.validRequestBus(instrRamValidRequestBus);
    instructionRam.dataInBus(instrRamDataInBus);

    instructionCache.memoryAddrBus(instrRamAddressSignal);
    instructionCache.memoryWeBus(instrRamWeBus);
    instructionCache.memoryValidRequestBus(instrRamValidRequestBus);
    instructionCache.memoryDataOutBus(instrRamDataInBus);

    // RAM -> Cache
    sc_signal<sc_dt::sc_bv<128>> instrRamDataOutSignal;
    sc_signal<bool> instrRamReadySignal;

    instructionRam.readyBus(instrRamReadySignal);
    instructionRam.dataOutBus(instrRamDataOutSignal);

    instructionCache.memoryReadyBus(instrRamReadySignal);
    instructionCache.memoryDataInBus(instrRamDataOutSignal);

    cpu.clock(clk);
    dataCache.clock(clk);
    instructionCache.clock(clk);
    dataRam.clock(clk);
    instructionRam.clock(clk);

    // Create tracefile if option is set
    sc_trace_file* trace;
    if (tracefile != NULL) {
        trace = sc_create_vcd_trace_file(tracefile);

        // Data Cache signals
        sc_trace(trace, cpuWeSignal, "cpuWeSignal");
        sc_trace(trace, cpuAddressSignal, "cpuAddressSignal");
        sc_trace(trace, cpuDataOutSignal, "cpuDataOutSignal");
        sc_trace(trace, cpuValidDataRequestSignal, "cpuValidDataRequestSignal");
        sc_trace(trace, cpuDataInSignal, "cpuDataInSignal");
        sc_trace(trace, cpuDataReadySignal, "cpuDataReadySignal");

        sc_trace(trace, ramWeSignal, "ramWeSignal");
        sc_trace(trace, ramAddressSignal, "ramAddressSignal");
        sc_trace(trace, ramDataInSignal, "ramDataInSignal");
        sc_trace(trace, ramValidRequestSignal, "ramValidRequestSignal");
        sc_trace(trace, ramDataOutSignal, "ramDataOutSignal");
        sc_trace(trace, ramReadySignal, "ramReadySignal");

        // Instruction Cache signals
        sc_trace(trace, validInstrRequestSignal, "validInstrRequestSignal");
        sc_trace(trace, pcSignal, "pcSignal");
        sc_trace(trace, instructionSignal, "instructionSignal");
        sc_trace(trace, instrReadySignal, "instrReadySignal");

        sc_trace(trace, instrRamAddressSignal, "instrRamAddressSignal");
        sc_trace(trace, instrRamWeBus, "instrRamWeBus");
        sc_trace(trace, instrRamValidRequestBus, "instrRamValidRequestBus");
        sc_trace(trace, instrRamDataInBus, "instrRamDataInBus");
        sc_trace(trace, instrRamDataOutSignal, "instrRamDataOutSignal");
        sc_trace(trace, instrRamReadySignal, "instrRamReadySignal");
    }

    sc_start(cycles, SC_NS);

    if (tracefile != NULL) {
        sc_close_vcd_trace_file(trace);
    }

    return Result{
        pcSignal >= numRequests
            ? cpu.getElapsedCycleCount()
            : SIZE_MAX,      // TODO: What if last request is not finished yet, but pc is already at numRequests
        dataCache.missCount, // TODO: What if a reuqest is stopped in the middle of processing, should the MISS/HIT be
                             // counted?
        dataCache.hitCount,
        1 // TODO: primitiveGateCount
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

int sc_main(__attribute__((unused)) int argc, __attribute__((unused)) char* argv[]) {
    std::cout << "ERROR: call to sc_main method\n";
    return 1;
}