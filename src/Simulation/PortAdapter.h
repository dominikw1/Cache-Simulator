#pragma once
#include <systemc>

// T has to be the size of a sc_bv and U has to be the size of a (not strictly) smaller sc_bv
template <typename U, typename T> SC_MODULE(PortAdapter) {
  public:
    sc_Core::sc_out<sc_dt::sc_bv<T>> in;
    sc_core::sc_in<sc_dt::sc_bv<U>> out;

    SC_CTOR(PortAdapter) {
        using namespace sc_core;
        SC_METHOD(update);
        sensitive << in;
    }

  private:
    void update() { out.write(in.range(U - 1, 0)); }
};