#pragma once

#include <map>
#include <systemc>
#include <unordered_map>

SC_MODULE(RAM) {
  public:
    // Global Clock
    sc_core::sc_in<bool> clock{"Global_Clock"};

    sc_core::sc_in<bool> validRequestBus{"validRequestBus"};
    sc_core::sc_in<bool> weBus{"weBus"};
    sc_core::sc_in<std::uint32_t> addressBus{"addressBus"};
    sc_core::sc_in<std::uint32_t> dataInBus{"dataInBus"};

    sc_core::sc_out<sc_dt::sc_bv<128>> dataOutBus{"dataOutBus"};
    sc_core::sc_out<bool> readyBus{"readyBus"};

  private:
    std::unordered_map<std::uint32_t, std::uint8_t> dataMemory{};
    std::uint32_t cyclesPassedInRequest = 0;
    std::uint32_t numRequestsPerformed;
    std::uint32_t latency;
    std::uint32_t wordsPerRead;

    SC_CTOR(RAM) {}

  public:
    RAM(sc_core::sc_module_name name, std::uint32_t latency, std::uint32_t wordsPerRead)
        : sc_module{name}, latency{latency}, wordsPerRead{wordsPerRead} {
        SC_THREAD(provideData);
        sensitive << clock.pos();
        dont_initialize();
    }

    std::uint8_t readByteFromMem(std::uint32_t addr) { return dataMemory[addr]; }

    void provideData() {
        while (true) {
            if (!validRequestBus.read())
                continue;

            readyBus.write(false);

            // Wait out latency
            while (cyclesPassedInRequest < latency) {
                wait(clock.posedge_event());
                ++cyclesPassedInRequest;
            }

            if (weBus.read()) {
                // Writing happens in one cycle -> one able to write 32 bits
                dataMemory[addressBus.read()] = (dataInBus.read() & ((1 << 8) - 1));
                dataMemory[addressBus.read() + 1] = (dataInBus.read() >> 8) & ((1 << 8) - 1);
                dataMemory[addressBus.read() + 2] = (dataInBus.read() >> 16) & ((1 << 8) - 1);
                dataMemory[addressBus.read() + 3] = (dataInBus.read() >> 24) & ((1 << 8) - 1);
                readyBus.write(true);
                wait(clock.posedge_event());
            } else {
                // Reading takes wordsPerRead cycles
                sc_dt::sc_bv<128> readData;
                for (std::size_t i = 0; i < wordsPerRead; ++i) {
                    // 128 / 8 -> 16
                    for (std::size_t byte = 0; byte < 16; ++byte) {
                        readData.range(8 * byte + 7, 8 * byte) = readByteFromMem(addressBus.read() + i * 16 + byte);
                    }
                    dataOutBus.write(readData);

                    // Next word is ready and then wait for next cycle to continue reading
                    readyBus.write(true);
                    wait(clock.posedge_event());
                }
            }

            ++numRequestsPerformed;
        }
    }
};
