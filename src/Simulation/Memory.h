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
    std::uint32_t numRequestsPerformed;
    std::uint32_t latency;
    std::uint32_t wordsPerRead;

    SC_CTOR(RAM) {}

#ifdef RAM_DEBUG
public:
#endif
    std::unordered_map<std::uint32_t, std::uint8_t> dataMemory{};


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
            wait(clock.posedge_event());
            readyBus.write(false);

            if (!validRequestBus.read())
                continue;
           // std::cout << "RAM: Got request at " << sc_core::sc_time_stamp() << "\n";

            // Wait out latency
            //std::cout<<latency<<std::endl;
            for (std::size_t cycles = 0; cycles < latency; ++cycles) {
                wait(clock.posedge_event());
            }
           // std::cout << "RAM: done with latency at " << sc_core::sc_time_stamp() << "\n";

            if (weBus.read()) {
                // Writing happens in one cycle -> one able to write 32 bits
                dataMemory[addressBus.read()] = (dataInBus.read() & ((1 << 8) - 1));
                dataMemory[addressBus.read() + 1] = (dataInBus.read() >> 8) & ((1 << 8) - 1);
                dataMemory[addressBus.read() + 2] = (dataInBus.read() >> 16) & ((1 << 8) - 1);
                dataMemory[addressBus.read() + 3] = (dataInBus.read() >> 24) & ((1 << 8) - 1);
                readyBus.write(true);
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

                    if (i != wordsPerRead - 1) {
                        wait();
                    }
                }
            }
          //  std::cout << "RAM done with request at " << sc_core::sc_time_stamp() << "\n";

            ++numRequestsPerformed;
        }
    }
};
