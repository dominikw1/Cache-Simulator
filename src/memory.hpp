
#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <systemc>
#include "systemc.h"

using namespace sc_core;


SC_MODULE(MEMORY) {
        sc_in<uint32_t> address;
        sc_in<sc_bv<1>> access;
        sc_port<uint32_t> data;

        u_int32[] memory;

        SC_TOR(MEMORY);

        MEMORY(sc_module_name name, int size):sc_module(name) {
            memory = new uint32_t[size];
        }

};

#endif
