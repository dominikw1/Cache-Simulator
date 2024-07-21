#include "../src/Simulation/PortAdapter.h"
#include <gtest/gtest.h>
using namespace sc_core;

TEST(PortAdapterTests, PortAdapterWorksAtInit) {
    PortAdapter<10, std::uint8_t, 8> TenToEightAdapter{"adapter"};
    sc_signal<sc_dt::sc_bv<10>> in;
    sc_signal<std::uint8_t> out;
    TenToEightAdapter.in.bind(in);
    TenToEightAdapter.out.bind(out);
    in.write((1 << 10) - 1);
    sc_start();
    ASSERT_EQ(TenToEightAdapter.out.read(), (1 << 8) - 1);
}