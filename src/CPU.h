#pragma once

#include "systemc.h"
#include <systemc>

#include "ReadOnlySpan.h"
#include "cache.hpp"
#include <cstdint>

SC_MODULE(CPU) {
  private:
    sc_core::sc_in<bool> clk;
    sc_core::sc_out<bool> weBus;
    sc_core::sc_out<std::uint32_t> addressBus;
    sc_core::sc_out<std::uint32_t> dataBus;

    sc_core::sc_signal<bool> weSignal;
    sc_core::sc_signal<std::uint32_t> addressSignal;
    sc_core::sc_signal<std::uint32_t> dataSignal;

    ReadOnlySpan<Request> requests;
    const std::uint32_t latency;

  public:
    CPU(::sc_core::sc_module_name name, std::uint32_t latency, ReadOnlySpan<Request> requests);
    typedef CPU SC_CURRENT_USER_MODULE;

  private:
    void update();
};