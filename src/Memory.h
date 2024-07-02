
#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <systemc>
#include <map>

using namespace sc_core;


SC_MODULE(MEMORY) {
    sc_in<uint32_t> addressBus;
    sc_in<int> weBus; // 0 -> read, 1 -> write
    sc_port<sc_signal<uint32_t>> dataBus;

    std::map<uint32_t, uint8_t> memory;
    unsigned int latency;

    SC_CTOR(MEMORY);

    MEMORY(sc_module_name name, unsigned int latency) : sc_module(name) {
        this->latency = latency;

        SC_THREAD(update);
        sensitive << weBus;
    }

    void update() {
        wait(latency);
        if (weBus) {
            uint32_t* a = splitUpAddress(addressBus);
            uint8_t* d = splitUpData(dataBus->read());
            for (int i = 0; i < 4; ++i) {
                memory[a[i]] = d[i];
            }
        } else {
            uint32_t* a = splitUpAddress(addressBus);
            uint8_t d[4];
            for (int i = 0; i < 4; ++i) {
                d[i] = memory[a[i]];
            }

            dataBus->write(combineData(d));
        }
    }

    uint32_t* splitUpAddress(uint32_t address) {
        return new uint32_t[]{address, address + 1, address + 2, address + 3};
    }

    uint8_t* splitUpData(uint32_t data) {
        uint8_t bytes[4];

        bytes[0] = (data >> 0) & ((1 << 8) - 1);
        bytes[1] = (data >> 8) & ((1 << 8) - 1);
        bytes[2] = (data >> 16) & ((1 << 8) - 1);
        bytes[3] = (data >> 24) & ((1 << 8) - 1);

        return bytes;
    }

    uint32_t combineData(uint8_t* dataParts) {
        return (dataParts[0] << 0) +
               (dataParts[1] << 8) +
               (dataParts[2] << 16) +
               (dataParts[3] << 24);
    }


};

#endif
