#pragma once

#include "../Request.h"
#include "Cache.h"

#include "Policy/FIFOPolicy.h"
#include <memory>
#include <systemc>

SC_MODULE(InstructionCache) {
  private:
    const std::uint32_t cacheLineNum;
    const std::uint32_t cacheLineSize;
    const std::uint32_t cacheLatency;

    Cache<MappingType::Direct> cache{"Cache", cacheLineNum, cacheLineSize, cacheLatency, nullptr};
    std::vector<Request> instructions;

    SC_CTOR(InstructionCache);

  public:
    sc_core::sc_in<bool> SC_NAMED(clock);

    // Instruction Cache -> CPU
    sc_core::sc_out<Request> SC_NAMED(instructionBus);
    sc_core::sc_out<bool> SC_NAMED(instrReadyBus);

    // CPU -> Instruction Cache
    sc_core::sc_in<bool> SC_NAMED(validInstrRequestBus);
    sc_core::sc_in<std::uint32_t> SC_NAMED(pcBus);

    // Cache -> RAM
    sc_core::sc_out<std::uint32_t> SC_NAMED(memoryAddrBus);
    sc_core::sc_out<std::uint32_t> SC_NAMED(memoryDataOutBus);
    sc_core::sc_out<bool> SC_NAMED(memoryWeBus);
    sc_core::sc_out<bool> SC_NAMED(memoryValidRequestBus);

    // RAM -> Cache
    sc_core::sc_in<sc_dt::sc_bv<128>> SC_NAMED(memoryDataInBus);
    sc_core::sc_in<bool> SC_NAMED(memoryReadyBus);

  private:
    sc_core::sc_signal<bool> SC_NAMED(instrWeSignal, false);       // always false
    sc_core::sc_signal<std::uint32_t> SC_NAMED(instrDataInSignal); // never read

    // All other ports can be directly connected to internal cache
    sc_core::sc_signal<std::uint32_t> SC_NAMED(cacheDataOutSignal);
    sc_core::sc_signal<bool> SC_NAMED(validInstrRequestSignal);

  public:
    InstructionCache(sc_core::sc_module_name name, std::uint32_t cacheLines, std::uint32_t cacheLineSize,
                     std::uint32_t cacheLatency, std::vector<Request> instructions)
        : sc_module{name}, cacheLineNum{cacheLines}, cacheLineSize{cacheLineSize}, cacheLatency{cacheLatency},
          instructions{instructions} {

        using namespace sc_core;

        cache.clock(clock);

        cache.memoryAddrBus(memoryAddrBus);
        cache.memoryDataOutBus(memoryDataOutBus);
        cache.memoryWeBus(memoryWeBus);
        cache.memoryValidRequestBus(memoryValidRequestBus);

        cache.memoryReadyBus(memoryReadyBus);
        cache.memoryDataInBus(memoryDataInBus);

        cache.cpuDataOutBus(cacheDataOutSignal); // dummy - gets discarded
        cache.ready(instrReadyBus);

        cache.cpuAddrBus(pcBus);
        cache.cpuDataInBus(instrDataInSignal);
        cache.cpuWeBus(instrWeSignal);
        cache.cpuValidRequest(validInstrRequestSignal);

        SC_METHOD(provideInstruction);
        sensitive << cache.ready;

        SC_METHOD(interceptTooHighPCVal);
        sensitive << pcBus << validInstrRequestBus;
    }

#ifdef STRICT_INSTRUCTION_ORDER
    void setMemoryLatency(std::uint32_t latency) { cache.setMemoryLatency(latency); }
#endif

  private:
    void interceptTooHighPCVal() {
        validInstrRequestSignal.write(validInstrRequestBus.read() && pcBus.read() < instructions.size());
    }

    void provideInstruction() {
        if (cache.ready.read()) {
            instructionBus.write(instructions[pcBus.read()]);
        }
    }
};