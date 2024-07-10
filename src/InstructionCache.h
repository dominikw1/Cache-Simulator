#pragma once

#include "Cache.h"
#include "Request.h"

#include <systemc>

// optimsiation: instruction buffer
SC_MODULE(InstructionCache) {
  private:
    CACHE<MappingType::DIRECT_MAPPED> rawCache;
    std::vector<Request> instructions;

    SC_CTOR(InstructionCache);

    void decode() ;

    void fetch();

  public:
    // -> CPU
    sc_core::sc_out<Request> instructionBus;
    sc_core::sc_out<bool> instrReadyBus;
    // <- CPU
    sc_core::sc_in<bool> validInstrRequestBus;
    sc_core::sc_in<std::uint32_t> pcBus;
    // -> RAM (same as cache)
    // <- RAM (Dummys, we do not write)
};