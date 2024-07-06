#include "CPU.h"
#include <iostream>
using namespace sc_core;

CPU::CPU(sc_module_name name) : sc_module{name} {
    SC_METHOD(dispatchInstruction);
    sensitive << instrReadyBus.pos();
    dont_initialize();

    SC_METHOD(receiveData);
    sensitive << dataReadyBus.pos();
    dont_initialize();

    SC_METHOD(requestInstruction);
    sensitive << cycleDone;
    // initialize to start off!
}

void CPU::dispatchInstruction() {
    currInstruction = Request{.addr = instrAddressBus.read(), .data = instrDataOutBus.read(), .we = instrWeBus.read()};
    weBus.write(currInstruction.we); // maybe bind these ports directly?
    addressBus.write(currInstruction.addr);
    dataOutBus.write(currInstruction.data);
}

void CPU::receiveData() {
    if (currInstruction.we) {
        std::cout << "Successfully wrote " << currInstruction.data << " to location " << currInstruction.addr
                  << std::endl;
    } else {
        std::cout << "Successfully read " << dataInBus.read() << " from address " << currInstruction.addr << std::endl;
    }
    cycleDone.notify(SC_ZERO_TIME);
}

void CPU::requestInstruction() { pcBus.write(program_counter); }
