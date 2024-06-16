#include "CPU.h"

using namespace sc_core;

CPU::CPU(sc_module_name name, std::uint32_t latency, const ReadOnlySpan<Request> requests)
    : sc_module{name}, latency{latency}, requests{requests} {
    weBus.bind(weSignal);
    addressBus.bind(addressSignal);
    dataBus.bind(dataSignal);
    SC_THREAD(update);
    sensitive << clk.pos();
}

void CPU::update() {
    weSignal.write(requests.top().we);
    addressSignal.write(requests.top().addr);
    dataSignal.write(requests.top().data);
    ++requests;
    wait(latency);
}