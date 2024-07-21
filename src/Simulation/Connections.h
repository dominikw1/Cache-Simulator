#pragma once

#include <systemc>

#include "../Request.h"
#include "CPU.h"
#include "Cache.h"
#include "InstructionCache.h"
#include "PortAdapter.h"
#include "RAM.h"

struct Connections {
    sc_core::sc_clock SC_NAMED(clk, sc_core::sc_time(1, sc_core::SC_NS));

    // Data Cache
    // CPU -> Cache
    sc_core::sc_signal<std::uint32_t> SC_NAMED(CPU_to_dataCache_Address);
    sc_core::sc_signal<std::uint32_t> SC_NAMED(CPU_to_dataCache_Data);
    sc_core::sc_signal<bool> SC_NAMED(CPU_to_dataCache_WE);
    sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS> SC_NAMED(CPU_to_dataCache_Valid_Request);

    // Cache -> CPU
    sc_core::sc_signal<std::uint32_t> SC_NAMED(dataCache_to_CPU_Data);
    sc_core::sc_signal<bool> SC_NAMED(dataCache_to_CPU_Ready);

    // Cache -> RAM
    sc_core::sc_signal<std::uint32_t, sc_core::SC_MANY_WRITERS> SC_NAMED(dataCache_to_dataRAM_Address);
    sc_core::sc_signal<std::uint32_t> SC_NAMED(dataCache_to_dataRAM_Data);
    sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS> SC_NAMED(dataCache_to_dataRAM_WE);
    sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS> SC_NAMED(dataCache_to_dataRAM_Valid_Request);

    // RAM -> Cache
    sc_core::sc_signal<sc_dt::sc_bv<128>> SC_NAMED(dataRAM_to_dataCache_Data);
    sc_core::sc_signal<bool> SC_NAMED(dataRAM_to_dataCache_Ready);

    // Instruction Cache
    // CPU -> Cache
    sc_core::sc_signal<std::uint32_t> SC_NAMED(CPU_to_instrCache_PC);
    sc_core::sc_signal<bool> SC_NAMED(CPU_to_instrCache_Valid_Request);

    // Cache -> CPU
    sc_core::sc_signal<Request> SC_NAMED(instrCache_to_CPU_Instruction);
    sc_core::sc_signal<bool> SC_NAMED(instrCache_to_CPU_Ready);

    // Cache -> RAM
    sc_core::sc_signal<std::uint32_t> SC_NAMED(instrCache_to_instrRAM_Address);
    sc_core::sc_signal<std::uint32_t> SC_NAMED(instrCache_to_instrRAM_Data);
    sc_core::sc_signal<bool> SC_NAMED(instrCache_to_instrRAM_WE);
    sc_core::sc_signal<bool> SC_NAMED(instrCache_to_instrRAM_Valid_Request);

    // RAM -> Cache
    sc_core::sc_signal<sc_dt::sc_bv<128>> SC_NAMED(instrRAM_to_instrCache_Data);
    sc_core::sc_signal<bool> SC_NAMED(instrRAM_to_instrCache_Ready);
};

template <typename DataCacheType>
inline std::unique_ptr<Connections> connectComponentsNoCache(CPU& cpu, RAM& dataRam, RAM& instructionRam,
                                                             InstructionCache& instructionCache,
                                                             DataCacheType& dataCache) {
    auto connections = std::make_unique<Connections>();
    // we reuse the signals meant to be between cpu and cache
    cpu.addressBus(connections->CPU_to_dataCache_Address);
    cpu.dataOutBus(connections->CPU_to_dataCache_Data);
    cpu.weBus(connections->CPU_to_dataCache_WE);
    cpu.validDataRequestBus(connections->CPU_to_dataCache_Valid_Request);

    cpu.dataInBus(connections->dataCache_to_CPU_Data);
    cpu.dataReadyBus(connections->dataCache_to_CPU_Ready);

    dataRam.addressBus(connections->CPU_to_dataCache_Address);
    dataRam.dataInBus(connections->CPU_to_dataCache_Data);
    dataRam.weBus(connections->CPU_to_dataCache_WE);
    dataRam.validRequestBus(connections->CPU_to_dataCache_Valid_Request);

    dataRam.readyBus(connections->dataCache_to_CPU_Ready);
    dataRam.dataOutBus(connections->dataRAM_to_dataCache_Data);

    // use adapter
    PortAdapter<128, std::uint32_t, 32> SC_NAMED(adapter);
    adapter.in(connections->dataRAM_to_dataCache_Data);
    adapter.out(connections->dataCache_to_CPU_Data);

    // Instruction Cache - we need that for decoding
    // CPU -> Cache
    cpu.pcBus(connections->CPU_to_instrCache_PC);
    cpu.validInstrRequestBus(connections->CPU_to_instrCache_Valid_Request);

    instructionCache.pcBus(connections->CPU_to_instrCache_PC);
    instructionCache.validInstrRequestBus(connections->CPU_to_instrCache_Valid_Request);

    // Cache -> CPU
    cpu.instrBus(connections->instrCache_to_CPU_Instruction);
    cpu.instrReadyBus(connections->instrCache_to_CPU_Ready);

    instructionCache.instructionBus(connections->instrCache_to_CPU_Instruction);
    instructionCache.instrReadyBus(connections->instrCache_to_CPU_Ready);

    // Cache -> RAM
    instructionRam.addressBus(connections->instrCache_to_instrRAM_Address);
    instructionRam.dataInBus(connections->instrCache_to_instrRAM_Data);
    instructionRam.weBus(connections->instrCache_to_instrRAM_WE);
    instructionRam.validRequestBus(connections->instrCache_to_instrRAM_Valid_Request);

    instructionCache.memoryAddrBus(connections->instrCache_to_instrRAM_Address);
    instructionCache.memoryDataOutBus(connections->instrCache_to_instrRAM_Data);
    instructionCache.memoryWeBus(connections->instrCache_to_instrRAM_WE);
    instructionCache.memoryValidRequestBus(connections->instrCache_to_instrRAM_Valid_Request);

    // RAM -> Cache
    instructionRam.dataOutBus(connections->instrRAM_to_instrCache_Data);
    instructionRam.readyBus(connections->instrRAM_to_instrCache_Ready);

    instructionCache.memoryDataInBus(connections->instrRAM_to_instrCache_Data);
    instructionCache.memoryReadyBus(connections->instrRAM_to_instrCache_Ready);

    cpu.clock(connections->clk);
    instructionCache.clock(connections->clk);
    dataRam.clock(connections->clk);
    instructionRam.clock(connections->clk);

    sc_core::sc_signal<std::uint32_t> SC_NAMED(DUMMY_dataCache_Address);
    sc_core::sc_signal<std::uint32_t> SC_NAMED(DUMMY_dataCache_Data_IN);
    sc_core::sc_signal<bool> SC_NAMED(DUMMY_dataCache_WE);
    sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS> SC_NAMED(DUMMY_dataCache_Valid_Request);
    dataCache.cpuAddrBus(DUMMY_dataCache_Address);
    dataCache.cpuDataOutBus(DUMMY_dataCache_Data_IN);
    dataCache.cpuWeBus(DUMMY_dataCache_WE);
    dataCache.cpuValidRequest(DUMMY_dataCache_Valid_Request);

    sc_core::sc_signal<std::uint32_t> SC_NAMED(DUMMY_CPU_Data);
    sc_core::sc_signal<bool> SC_NAMED(DUMMY_CPU_Ready);
    dataCache.cpuDataInBus(DUMMY_CPU_Data);
    dataCache.ready(DUMMY_CPU_Ready);

    sc_core::sc_signal<std::uint32_t, sc_core::SC_MANY_WRITERS> SC_NAMED(DUMMY_dataRAM_Address);
    sc_core::sc_signal<std::uint32_t> SC_NAMED(DUMMY_dataRAM_Data);
    sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS> SC_NAMED(DUMMY_dataRAM_WE);
    sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS> SC_NAMED(DUMMY_dataRAM_Valid_Request);
    dataCache.memoryAddrBus(DUMMY_dataRAM_Address);
    dataCache.memoryWeBus(DUMMY_dataRAM_WE);
    dataCache.memoryDataOutBus(DUMMY_dataRAM_Data);
    dataCache.memoryValidRequestBus(DUMMY_dataRAM_Valid_Request);

    sc_core::sc_signal<sc_dt::sc_bv<128>> SC_NAMED(DUMMY_dataCache_Data_OUT);
    sc_core::sc_signal<bool> SC_NAMED(DUMMY_dataCache_Ready);
    dataCache.memoryReadyBus(DUMMY_dataCache_Ready);
    dataCache.memoryDataInBus(DUMMY_dataCache_Data_OUT);

    return connections;
}

template <MappingType mappingType>
inline std::unique_ptr<Connections> connectComponents(CPU& cpu, RAM& dataRam, RAM& instructionRam,
                                                      Cache<mappingType>& dataCache,
                                                      InstructionCache& instructionCache) {
    auto connections = std::make_unique<Connections>();
    // Data Cache
    // CPU -> Cache
    cpu.addressBus(connections->CPU_to_dataCache_Address);
    cpu.dataOutBus(connections->CPU_to_dataCache_Data);
    cpu.weBus(connections->CPU_to_dataCache_WE);
    cpu.validDataRequestBus(connections->CPU_to_dataCache_Valid_Request);

    dataCache.cpuAddrBus(connections->CPU_to_dataCache_Address);
    dataCache.cpuDataInBus(connections->CPU_to_dataCache_Data);
    dataCache.cpuWeBus(connections->CPU_to_dataCache_WE);
    dataCache.cpuValidRequest(connections->CPU_to_dataCache_Valid_Request);

    // Cache -> CPU
    cpu.dataInBus(connections->dataCache_to_CPU_Data);
    cpu.dataReadyBus(connections->dataCache_to_CPU_Ready);

    dataCache.cpuDataOutBus(connections->dataCache_to_CPU_Data);
    dataCache.ready(connections->dataCache_to_CPU_Ready);

    // Cache -> RAM
    dataRam.addressBus(connections->dataCache_to_dataRAM_Address);
    dataRam.dataInBus(connections->dataCache_to_dataRAM_Data);
    dataRam.weBus(connections->dataCache_to_dataRAM_WE);
    dataRam.validRequestBus(connections->dataCache_to_dataRAM_Valid_Request);

    dataCache.memoryAddrBus(connections->dataCache_to_dataRAM_Address);
    dataCache.memoryDataOutBus(connections->dataCache_to_dataRAM_Data);
    dataCache.memoryWeBus(connections->dataCache_to_dataRAM_WE);
    dataCache.memoryValidRequestBus(connections->dataCache_to_dataRAM_Valid_Request);

    // RAM -> Cache
    dataRam.dataOutBus(connections->dataRAM_to_dataCache_Data);
    dataRam.readyBus(connections->dataRAM_to_dataCache_Ready);

    dataCache.memoryDataInBus(connections->dataRAM_to_dataCache_Data);
    dataCache.memoryReadyBus(connections->dataRAM_to_dataCache_Ready);

    // Instruction Cache
    // CPU -> Cache
    cpu.pcBus(connections->CPU_to_instrCache_PC);
    cpu.validInstrRequestBus(connections->CPU_to_instrCache_Valid_Request);

    instructionCache.pcBus(connections->CPU_to_instrCache_PC);
    instructionCache.validInstrRequestBus(connections->CPU_to_instrCache_Valid_Request);

    // Cache -> CPU
    cpu.instrBus(connections->instrCache_to_CPU_Instruction);
    cpu.instrReadyBus(connections->instrCache_to_CPU_Ready);

    instructionCache.instructionBus(connections->instrCache_to_CPU_Instruction);
    instructionCache.instrReadyBus(connections->instrCache_to_CPU_Ready);

    // Cache -> RAM
    instructionRam.addressBus(connections->instrCache_to_instrRAM_Address);
    instructionRam.dataInBus(connections->instrCache_to_instrRAM_Data);
    instructionRam.weBus(connections->instrCache_to_instrRAM_WE);
    instructionRam.validRequestBus(connections->instrCache_to_instrRAM_Valid_Request);

    instructionCache.memoryAddrBus(connections->instrCache_to_instrRAM_Address);
    instructionCache.memoryDataOutBus(connections->instrCache_to_instrRAM_Data);
    instructionCache.memoryWeBus(connections->instrCache_to_instrRAM_WE);
    instructionCache.memoryValidRequestBus(connections->instrCache_to_instrRAM_Valid_Request);

    // RAM -> Cache
    instructionRam.dataOutBus(connections->instrRAM_to_instrCache_Data);
    instructionRam.readyBus(connections->instrRAM_to_instrCache_Ready);

    instructionCache.memoryDataInBus(connections->instrRAM_to_instrCache_Data);
    instructionCache.memoryReadyBus(connections->instrRAM_to_instrCache_Ready);

    cpu.clock(connections->clk);
    dataCache.clock(connections->clk);
    instructionCache.clock(connections->clk);
    dataRam.clock(connections->clk);
    instructionRam.clock(connections->clk);

    return connections;
}