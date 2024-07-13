#pragma once

#include "Request.h"
#include <atomic>
#include <cstdint>
#include <systemc>

SC_MODULE(CPU) {
  public:
    // CPU -> Cache
    sc_core::sc_out<bool> weBus;
    sc_core::sc_out<bool> validDataRequestBus;
    sc_core::sc_out<std::uint32_t> addressBus;
    sc_core::sc_out<std::uint32_t> dataOutBus;

    // Cache -> CPU
    sc_core::sc_in<std::uint32_t> dataInBus;
    sc_core::sc_in<bool> dataReadyBus;

    // Instr Cache -> CPU
    sc_core::sc_in<Request> instrBus;
    sc_core::sc_in<bool> instrReadyBus;

    // CPU -> Instr Cache
    sc_core::sc_out<bool> validInstrRequestBus;
    sc_core::sc_out<std::uint32_t> pcBus;

    sc_core::sc_in<bool> clock;

  private:
    std::uint64_t program_counter = 0;
    Request currInstruction;
    // sc_core::sc_event dataCycleDone;
    sc_core::sc_event instructionCycleDone;

    std::atomic<std::uint64_t> cycleNum{0ull};
    std::uint64_t lastCycleWhereWorkWasDone = 0;

  public:
    SC_CTOR(CPU);
    constexpr std::uint64_t getElapsedCycleCount() const noexcept { return lastCycleWhereWorkWasDone; };

  private:
    void startWorking(); // temporary, find a better solution!!!
    void dispatchInstruction();
    void receiveData();
    void requestInstruction();
};