#include "CPU.h"
#include <iostream>
using namespace sc_core;

CPU::CPU(sc_module_name name) : sc_module{name} {
    SC_METHOD(dispatchInstruction);
    sensitive << clock.pos();
    dont_initialize();

    SC_METHOD(receiveData);
    sensitive << dataReadyBus.pos();
    dont_initialize();

    SC_METHOD(requestInstruction);
    sensitive << instructionCycleDone;
    dont_initialize();

    // initialize to start off!
    SC_THREAD(startWorking);
}

void CPU::dispatchInstruction() {
    ++cycleNum;
    if (instrReadyBus.read()) {
        validInstrRequestBus.write(false);
        currInstruction = instrBus;
        weBus.write(currInstruction.we); // maybe bind these ports directly?
        addressBus.write(currInstruction.addr);
        dataOutBus.write(currInstruction.data);
        validDataRequest.write(true);

        // kick off next instruction fetch
        instructionCycleDone.notify(SC_ZERO_TIME);
    }
}

void CPU::receiveData() {
    std::cout << "Receiving data" << std::endl;
    if (currInstruction.we) {
        std::cout << "Successfully wrote " << currInstruction.data << " to location " << currInstruction.addr
                  << std::endl;
    } else {
        std::cout << "Successfully read " << dataInBus.read() << " from address " << currInstruction.addr << std::endl;
    }
    lastCycleWhereWorkWasDone = cycleNum.load();
}

void CPU::requestInstruction() {
    validDataRequest.write(false);
    std::cout << "Requesting instruction " << program_counter << std::endl;
    pcBus.write(program_counter);
    validInstrRequestBus.write(true);
    ++program_counter; // TODO: Figure out how large our instructiosn are supposed to be
}

void CPU::startWorking() { instructionCycleDone.notify(SC_ZERO_TIME); }
