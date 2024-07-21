#include "RAM.h"

RAM::RAM(sc_core::sc_module_name name, std::uint32_t memoryLatency, std::uint32_t wordsPerRead)
    : sc_module{name}, memoryLatency{memoryLatency}, wordsPerRead{wordsPerRead} {
    SC_THREAD(provideData);
    sensitive << clock.pos();
    dont_initialize();
}

void RAM::provideData() noexcept {
    while (true) {
        wait();
        readyBus.write(false);

        if (!validRequestBus.read())
            continue;

        waitOutMemoryLatency();

        if (weBus.read()) {
            doWrite();
        } else {
            // Reading takes wordsPerRead cycles
            for (std::uint32_t i = 0; i < wordsPerRead; ++i) {
                readWord(i);

                // Don't have to wait for last word to be read here because of the wait at the beginning of the while
                if (i != wordsPerRead - 1) {
                    wait();
                }
            }
        }
    }
}

void RAM::readWord(std::uint32_t word) noexcept {
    sc_dt::sc_bv<128> readData;

    // 128 / 8 -> 16
    for (int byte = 0; byte < 16; ++byte) {
        readData.range(8 * byte + 7, 8 * byte) = readByteFromMem(addressBus.read() + word * 16 + byte);
    }
    dataOutBus.write(readData);

    // Next word is ready and then wait for next cycle to continue reading
    readyBus.write(true);
}

std::uint8_t RAM::readByteFromMem(std::uint32_t addr) noexcept { return dataMemory[addr]; }

void RAM::doWrite() noexcept {
    dataMemory[addressBus.read()] = (dataInBus.read() & ((1 << 8) - 1));
    dataMemory[addressBus.read() + 1] = (dataInBus.read() >> 8) & ((1 << 8) - 1);
    dataMemory[addressBus.read() + 2] = (dataInBus.read() >> 16) & ((1 << 8) - 1);
    dataMemory[addressBus.read() + 3] = (dataInBus.read() >> 24) & ((1 << 8) - 1);

    readyBus.write(true);
}

void RAM::waitOutMemoryLatency() noexcept {
    for (std::size_t i = 0; i < memoryLatency; ++i) {
        wait();
    }
}