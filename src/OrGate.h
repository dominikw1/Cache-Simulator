#pragma once
#include <systemc>

SC_MODULE(OrGate) {
  public:
    sc_core::sc_in<bool> SC_NAMED(a);
    sc_core::sc_in<bool> SC_NAMED(b);
    sc_core::sc_out<bool> SC_NAMED(out);
    SC_CTOR(OrGate) {
        using namespace sc_core;
        SC_METHOD(doOp);
        sensitive << a << b;
        dont_initialize();
    }

  private:
    void doOp() { out.write(a.read() || b.read()); }
};