
#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <systemc>
#include "systemc.h"

using namespace sc_core;


SC_MODULE(MEMORY) {
    sc_in<bool> clk;
    sc_in<uint32_t> addressBus;
    sc_in<int> weBus; // 0 -> read, 1 -> write
    sc_port<sc_signal<uint32_t>> dataBus;

    std::vector<uint32_t> memory;
    int latency;

    SC_CTOR(MEMORY);

    MEMORY(sc_module_name name, int latency) : sc_module(name) {
        this->latency = latency;

        SC_THREAD(update);
        sensitive << clk.pos();
    }

    void update() {
        wait(latency);
        if (weBus) {
            dataBus->write(memory[addressBus]);
        } else {
            memory[addressBus] = dataBus->read();
        }
    }


};

#endif
