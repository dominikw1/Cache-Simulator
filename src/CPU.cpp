#include "CPU.h"

using namespace sc_core;

CPU::CPU(sc_module_name name, std::uint32_t latency, const ReadOnlySpan<Request> requests)
    : sc_module{name}, latency{latency}, requests{requests} {
    SC_THREAD(update);
    sensitive << clk.pos();
}

void CPU::update() {
    wait(latency);
    if (weBus) {
        // dataBus->write(memory[addressBus]);
    } else {
        // memory[addressBus] = dataBus->read();
    }
}