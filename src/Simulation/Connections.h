#pragma once

#include <systemc>

#include "../Request.h"
#include "Cache.h"
#include "CPU.h"
#include "Memory.h"
#include "InstructionCache.h"

struct Connections {
    sc_core::sc_clock clk{"Clock", sc_core::sc_time(1, sc_core::SC_NS)};

    // Data Cache
    // CPU -> Cache
    sc_core::sc_signal<bool> SC_NAMED(cpuWeSignal);
    sc_core::sc_signal<std::uint32_t> SC_NAMED(cpuAddressSignal);
    sc_core::sc_signal<std::uint32_t> SC_NAMED(cpuDataOutSignal);
    sc_core::sc_signal<bool> SC_NAMED(cpuValidDataRequestSignal);

    // Cache -> CPU
    sc_core::sc_signal<std::uint32_t> SC_NAMED(cpuDataInSignal);
    sc_core::sc_signal<bool> SC_NAMED(cpuDataReadySignal);

    // Cache -> RAM
    sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS> SC_NAMED(ramWeSignal);
    sc_core::sc_signal<std::uint32_t, sc_core::SC_MANY_WRITERS> SC_NAMED(ramAddressSignal);
    sc_core::sc_signal<std::uint32_t> SC_NAMED(ramDataInSignal);
    sc_core::sc_signal<bool, sc_core::SC_MANY_WRITERS> SC_NAMED(ramValidRequestSignal);

    // RAM -> Cache
    sc_core::sc_signal<sc_dt::sc_bv<128>> SC_NAMED(ramDataOutSignal);
    sc_core::sc_signal<bool> SC_NAMED(ramReadySignal);

    // Instruction Cache
    // CPU -> Cache
    sc_core::sc_signal<bool> SC_NAMED(validInstrRequestSignal);
    sc_core::sc_signal<std::uint32_t> SC_NAMED(pcSignal);

    // Cache -> CPU
    sc_core::sc_signal<Request> SC_NAMED(instructionSignal);
    sc_core::sc_signal<bool> SC_NAMED(instrReadySignal);

    // Cache -> RAM
    sc_core::sc_signal<std::uint32_t> SC_NAMED(instrRamAddressSignal);
    sc_core::sc_signal<bool> SC_NAMED(instrRamWeBus);
    sc_core::sc_signal<bool> SC_NAMED(instrRamValidRequestBus);
    sc_core::sc_signal<std::uint32_t> SC_NAMED(instrRamDataInBus);

    // RAM -> Cache
    sc_core::sc_signal<sc_dt::sc_bv<128>> SC_NAMED(instrRamDataOutSignal);
    sc_core::sc_signal<bool> SC_NAMED(instrRamReadySignal);
};

template<MappingType mappingType>
std::unique_ptr<Connections> connectComponents(CPU& cpu,
                                         RAM& dataRam, RAM& instructionRam,
                                         Cache<mappingType>& dataCache, InstructionCache& instructionCache) {
    auto connections = std::make_unique<Connections>();

    // Data Cache
    // CPU -> Cache
    cpu.weBus(connections->cpuWeSignal);
    cpu.addressBus(connections->cpuAddressSignal);
    cpu.dataOutBus(connections->cpuDataOutSignal);
    cpu.validDataRequestBus(connections->cpuValidDataRequestSignal);

    dataCache.cpuWeBus(connections->cpuWeSignal);
    dataCache.cpuAddrBus(connections->cpuAddressSignal);
    dataCache.cpuDataInBus(connections->cpuDataOutSignal);
    dataCache.cpuValidRequest(connections->cpuValidDataRequestSignal);

    // Cache -> CPU
    cpu.dataInBus(connections->cpuDataInSignal);
    cpu.dataReadyBus(connections->cpuDataReadySignal);

    dataCache.cpuDataOutBus(connections->cpuDataInSignal);
    dataCache.ready(connections->cpuDataReadySignal);

    // Cache -> RAM
    dataRam.weBus(connections->ramWeSignal);
    dataRam.addressBus(connections->ramAddressSignal);
    dataRam.validRequestBus(connections->ramValidRequestSignal);
    dataRam.dataInBus(connections->ramDataInSignal);

    dataCache.memoryWeBus(connections->ramWeSignal);
    dataCache.memoryAddrBus(connections->ramAddressSignal);
    dataCache.memoryValidRequestBus(connections->ramValidRequestSignal);
    dataCache.memoryDataOutBus(connections->ramDataInSignal);

    // RAM -> Cache
    dataRam.readyBus(connections->ramReadySignal);
    dataRam.dataOutBus(connections->ramDataOutSignal);

    dataCache.memoryReadyBus(connections->ramReadySignal);
    dataCache.memoryDataInBus(connections->ramDataOutSignal);

    // Instruction Cache
    // CPU -> Cache
    cpu.validInstrRequestBus(connections->validInstrRequestSignal);
    cpu.pcBus(connections->pcSignal);

    instructionCache.validInstrRequestBus(connections->validInstrRequestSignal);
    instructionCache.pcBus(connections->pcSignal);

    // Cache -> CPU
    cpu.instrBus(connections->instructionSignal);
    cpu.instrReadyBus(connections->instrReadySignal);

    instructionCache.instructionBus(connections->instructionSignal);
    instructionCache.instrReadyBus(connections->instrReadySignal);

    // Cache -> RAM
    instructionRam.addressBus(connections->instrRamAddressSignal);
    instructionRam.weBus(connections->instrRamWeBus);
    instructionRam.validRequestBus(connections->instrRamValidRequestBus);
    instructionRam.dataInBus(connections->instrRamDataInBus);

    instructionCache.memoryAddrBus(connections->instrRamAddressSignal);
    instructionCache.memoryWeBus(connections->instrRamWeBus);
    instructionCache.memoryValidRequestBus(connections->instrRamValidRequestBus);
    instructionCache.memoryDataOutBus(connections->instrRamDataInBus);

    // RAM -> Cache
    instructionRam.readyBus(connections->instrRamReadySignal);
    instructionRam.dataOutBus(connections->instrRamDataOutSignal);

    instructionCache.memoryReadyBus(connections->instrRamReadySignal);
    instructionCache.memoryDataInBus(connections->instrRamDataOutSignal);

    cpu.clock(connections->clk);
    dataCache.clock(connections->clk);
    instructionCache.clock(connections->clk);
    dataRam.clock(connections->clk);
    instructionRam.clock(connections->clk);

    return connections;
}