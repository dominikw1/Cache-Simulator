#include "CPU.h"

CPU::CPU(sc_core::sc_module_name name, Request* instructions) : sc_module{name}, instructions{instructions} {
    SC_THREAD(handleInstruction);
    sensitive << clock.pos();

    SC_THREAD(readInstruction);
    sensitive << clock.pos();
}

void CPU::handleInstruction() noexcept {
    while (true) {
        if (!skipAhead) {
            wait();
        } else {
            skipAhead = false;
        }

        if (instructionReady) {
            Request currentRequest = instrBus.read();
            addressBus.write(currentRequest.addr);
            dataOutBus.write(currentRequest.data);
            weBus.write(currentRequest.we);

            validDataRequestBus.write(true);

            // After reading current trigger reading of next instruction
            instructionReady = false;
            triggerNextInstructionRead.notify();

            waitForInstructionProcessing();

            lastCycleWhereWorkWasDone = sc_core::sc_time_stamp().value() / 1000;

            if (!currentRequest.we) {
                instructions[program_counter - 1].data = dataInBus;
            }
        }
    }
}

void CPU::readInstruction() noexcept {
    while (true) {
        wait();

        pcBus.write(program_counter);
        validInstrRequestBus.write(true);

        waitForInstruction();

        instructionReady = true;

        wait(triggerNextInstructionRead);
        ++program_counter; // Increase PC after the event has been triggered to reflect a correct state of the PC
    }
}

void CPU::waitForInstruction() noexcept {
    wait();
    validInstrRequestBus.write(false);
    while (!instrReadyBus) {
        wait();
    }
}

void CPU::waitForInstructionProcessing() noexcept {
    wait();
    validDataRequestBus.write(false);
    while (!dataReadyBus) {
        wait();
    }
    skipAhead = true;
}