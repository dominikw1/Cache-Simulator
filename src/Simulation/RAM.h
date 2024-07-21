#pragma once

#include <map>
#include <systemc>
#include <unordered_map>
#include <cstdint>

SC_MODULE(RAM) {
  public:
    // ====================================== External Ports  ======================================
    // Global Clock
    sc_core::sc_in<bool> SC_NAMED(clock);

    // -> RAM
    sc_core::sc_in<std::uint32_t> SC_NAMED(dataInBus);
    sc_core::sc_in<std::uint32_t> SC_NAMED(addressBus);
    sc_core::sc_in<bool> SC_NAMED(weBus);
    sc_core::sc_in<bool> SC_NAMED(validRequestBus);

    // RAM ->
    sc_core::sc_out<sc_dt::sc_bv<128>> SC_NAMED(dataOutBus);
    sc_core::sc_out<bool> SC_NAMED(readyBus);

  private:
    std::uint32_t memoryLatency;
    std::uint32_t wordsPerRead;


#ifdef RAM_DEBUG
  public:
#endif
    std::unordered_map<std::uint32_t, std::uint8_t> dataMemory{};


  public:
    RAM(sc_core::sc_module_name name, std::uint32_t memoryLatency, std::uint32_t wordsPerRead);

  private:
    SC_CTOR(RAM) {}

    // ======================================= Main Handling =======================================
    /**
     * Sleeps until it receives a valid requests and than based on the requests either reads from the data memory
     * or writes to it.
     */
    void provideData() noexcept;
    /**
     * Reads byte at the given address from the data memory
     * @param  address to read
     * @return data to to corresponding address
     */
    std::uint8_t readByteFromMem(std::uint32_t addr) noexcept;
    /**
     * Splits up input int into 4 bytes and then writes it to data memory to the given address
     */
    void doWrite() noexcept;
    /**
     * Reads word into a 128 bit buffer byte by byte at the received address a with a offset of (128/8) * word
     * @param word offset * 16 to received address
     */
    void readWord(std::uint32_t word) noexcept;

    // ====================================== Waiting Helpers ======================================
    /**
     * Sleeps for memoryLatency cycles
     */
    void waitOutMemoryLatency() noexcept;
};
