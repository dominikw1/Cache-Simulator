
#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <map>
#include <systemc>

SC_MODULE(RAM) {
  public:
    sc_in<bool> validDataRequest;
    sc_in<bool> weBus;
    sc_in<std::uint32_t> addressBus;
    std::vector<sc_in<std::uint8_t>> dataInBusses;

    std::vector<sc_out<std::uint8_t>> dataOutBusses;
    sc_out<bool> readyBus;

  private:
    std::unordered_map<std::uint32_t, std::uint8_t> dataMemory{};
    const std::uint32_t interfaceSize;
    SC_CTOR(RAM) {}

  public:
    RAMMock(sc_module_name name, std::uint32_t interfaceSize, std::uint32_t latency)
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
            wait(latency, SC_NS);
            bool we = weBus.read();
            for (int i = 0; i < interfaceSize; ++i) {
                if (!we) {
                    dataOutBusses.at(i).write(readByteFromMem(addressBus.read() + i));
                } else {
                    dataMemory[i] = dataInBusses.at(i).read();
                }
            }
            readyBus.write(true);
            wait();
        }
    }
};
