#include <gtest/gtest.h>
#include "../src/Simulation/Memory.h"
#include <systemc>

using namespace sc_core;

class MemoryTests : public testing::Test {
protected:
    RAM ram{"RAM", 2, 1};

    sc_signal<bool> weSignal;
    sc_signal<std::uint32_t> dataInSignal;
    sc_signal<std::uint32_t> addressSignal;
    sc_signal<bool> validRequestSignal;

    sc_signal<sc_dt::sc_bv<128>> dataOutSignal;
    sc_signal<bool> readySignal;

    sc_clock clock{"clk", sc_time(1, SC_NS)};

    void SetUp() {
        ram.validRequestBus(validRequestSignal);
        ram.weBus(weSignal);
        ram.addressBus(addressSignal);
        ram.dataInBus(dataInSignal);

        ram.readyBus(readySignal);
        ram.dataOutBus(dataOutSignal);

        ram.clock.bind(clock);
    }
};

TEST_F(MemoryTests, MemoryWriteAndReadSameAddress) {
    auto value = 1234;
    auto valueInBytes = new std::uint8_t[]{static_cast<uint8_t>(value), static_cast<uint8_t>(value >> 8),
                                           static_cast<uint8_t>(value >> 16), static_cast<uint8_t>(value >> 24)};

    addressSignal = 0;
    dataInSignal = value;
    weSignal = true;
    validRequestSignal = true;

    sc_start(1, SC_MS);

    addressSignal = 0;
    weSignal = false;
    validRequestSignal = true;

    sc_start(1, SC_MS);

    ASSERT_EQ(dataOutSignal.read().range(7, 0), sc_dt::sc_bv<8>(valueInBytes[0]));
    ASSERT_EQ(dataOutSignal.read().range(15, 8), sc_dt::sc_bv<8>(valueInBytes[1]));
    ASSERT_EQ(dataOutSignal.read().range(23, 16), sc_dt::sc_bv<8>(valueInBytes[2]));
    ASSERT_EQ(dataOutSignal.read().range(31, 24), sc_dt::sc_bv<8>(valueInBytes[3]));
}

TEST_F(MemoryTests, MemoryOverwriteAddressWithZeroAndRead) {
    auto value = 1234;

    addressSignal = 0;
    dataInSignal = value;
    weSignal = true;
    validRequestSignal = true;

    sc_start(1, SC_MS);

    addressSignal = 1;
    dataInSignal = 0;
    weSignal = true;
    validRequestSignal = true;

    sc_start(1, SC_MS);

    addressSignal = 0;
    weSignal = false;
    validRequestSignal = true;

    sc_start(1, SC_MS);

    ASSERT_EQ(dataOutSignal.read().range(7, 0), sc_dt::sc_bv<8>(static_cast<uint8_t>(value)));
    ASSERT_EQ(dataOutSignal.read().range(15, 8), sc_dt::sc_bv<8>(0));
    ASSERT_EQ(dataOutSignal.read().range(23, 16), sc_dt::sc_bv<8>(0));
    ASSERT_EQ(dataOutSignal.read().range(31, 24), sc_dt::sc_bv<8>(0));
}

TEST_F(MemoryTests, ReadBlankAddress) {
    addressSignal = 0;
    weSignal = false;
    validRequestSignal = true;

    sc_start(1, SC_MS);

    ASSERT_EQ(dataOutSignal.read(), sc_dt::sc_bv<128>(0));
}