#pragma once

#include <systemc>

#include "ReadOnlySpan.h"
#include "Request.h"
#include <cstdint>

SC_MODULE(CPU) {
  public:
    // CPU -> Cache
    sc_core::sc_out<bool> weBus;
    sc_core::sc_out<bool> validDataRequest;
    sc_core::sc_out<std::uint32_t> addressBus;
    sc_core::sc_out<std::uint32_t> dataOutBus;

    // Cache -> CPU
    sc_core::sc_in<std::uint32_t> dataInBus;
    sc_core::sc_in<bool> dataReadyBus;

    // TODO: move decoding into CPU maybe
    // Instr Cache -> CPU
    sc_core::sc_in<bool> instrWeBus;
    sc_core::sc_in<std::uint32_t> instrAddressBus;
    sc_core::sc_in<std::uint32_t> instrDataOutBus;
    sc_core::sc_in<bool> instrReadyBus;

    // CPU -> Instr Cache
    sc_core::sc_out<bool> validInstrRequest;
    sc_core::sc_out<std::uint32_t> pcBus;


    sc_core::sc_in<bool> clock;
  private:
    std::uint64_t program_counter = 0;
    Request currInstruction;
   // sc_core::sc_event dataCycleDone;
    sc_core::sc_event instructionCycleDone;
  public:
    CPU(::sc_core::sc_module_name name);
    typedef CPU SC_CURRENT_USER_MODULE;

  private:
    void startWorking(); // temporary, find a better solution!!!
    void dispatchInstruction();
    void receiveData();
    void requestInstruction();
};