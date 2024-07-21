#pragma once
#include <cstdint>
#include <systemc>

// T has to be the size of a sc_bv and U is a C++ unsigned type with size F
template <std::uint64_t T, typename U, std::uint64_t F> SC_MODULE(PortAdapter) {
  public:
    sc_core::sc_in<sc_dt::sc_bv<T>> in;
    sc_core::sc_out<U> out;

    SC_CTOR(PortAdapter) {
        using namespace sc_core;
        SC_METHOD(update);
        sensitive << in;
    }

  private:
    void update() { out.write(in.read().range(F - 1, 0).to_uint64()); }
};