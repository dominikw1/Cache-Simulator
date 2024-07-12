#pragma once

#include "Cache.h"
#include "Request.h"

#include "RandomPolicy.h"
#include <memory>
#include <systemc>

// optimsiation: instruction buffer
SC_MODULE(InstructionCache) {
  private:
    Cache<MappingType::Direct> cache{"Cache", 10, 64, 10, std::make_unique<RandomPolicy<std::uint32_t>>(10)};

    std::vector<Request> instructions;

    SC_CTOR(InstructionCache);

    void decode();

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