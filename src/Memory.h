#pragma once
#include <map>
#include <systemc>
#include <unordered_map>

SC_MODULE(RAM) {
  public:
    sc_core::sc_in<bool> validDataRequest;
    sc_core::sc_in<bool> weBus;
    sc_core::sc_in<std::uint32_t> addressBus;
    std::vector<sc_core::sc_in<std::uint8_t>> dataInBusses;

    std::vector<sc_core::sc_out<std::uint8_t>> dataOutBusses;
    sc_core::sc_out<bool> readyBus;

  private:
    std::unordered_map<std::uint32_t, std::uint8_t> dataMemory{};
    std::uint32_t interfaceSize;
    std::uint32_t latency;
    SC_CTOR(RAM) {}

  public:
    RAM(sc_core::sc_module_name name, std::uint32_t interfaceSize, std::uint32_t latency)
        : sc_module{name}, dataOutBusses(interfaceSize), dataInBusses(interfaceSize), interfaceSize{interfaceSize},
          latency{latency} {
        SC_THREAD(provideData);
        sensitive << validDataRequest.pos();
        dont_initialize();

        SC_METHOD(stopProviding);
        sensitive << validDataRequest.neg();
        dont_initialize();
    }

    void stopProviding() { readyBus.write(false); }

    std::uint8_t readByteFromMem(std::uint32_t addr) { return dataMemory[addr]; }

    void provideData() {
        while (true) {
            wait(latency, sc_core::SC_NS);
            bool we = weBus.read();
            for (int i = 0; i < interfaceSize; ++i) {
                if (!we) {
                    dataOutBusses.at(i).write(readByteFromMem(addressBus.read() + i));
                } else {
                    dataMemory[addressBus.read() + i] = dataInBusses.at(i).read();
                }
            }
            readyBus.write(true);
            wait();
        }
    }
};
