#pragma once

#include "../Request.h"
#include <cstdint>
#include <systemc>
#include <vector>

SC_MODULE(CPU) {
public:
    sc_core::sc_in<bool> SC_NAMED(clock);

    // CPU -> Cache
    sc_core::sc_out<bool> SC_NAMED(weBus);
    sc_core::sc_out<bool> SC_NAMED(validDataRequestBus);
    sc_core::sc_out<std::uint32_t> SC_NAMED(addressBus);
    sc_core::sc_out<std::uint32_t> SC_NAMED(dataOutBus);

    // Cache -> CPU
    sc_core::sc_in<std::uint32_t> SC_NAMED(dataInBus);
    sc_core::sc_in<bool> SC_NAMED(dataReadyBus);

    // Instr Cache -> CPU
    sc_core::sc_in<Request> SC_NAMED(instrBus);
    sc_core::sc_in<bool> SC_NAMED(instrReadyBus);

    // CPU -> Instr Cache
    sc_core::sc_out<bool> SC_NAMED(validInstrRequestBus);
    sc_core::sc_out<std::uint32_t> SC_NAMED(pcBus);

private:
    std::uint64_t program_counter = 0;

    std::uint64_t lastCycleWhereWorkWasDone = 0;
    std::uint64_t currCycle = 0;

    bool instructionReady = false;
    sc_core::sc_event triggerNextInstructionRead;

    Request* instructions;
    const std::uint64_t numInstructions;

    SC_CTOR(CPU);

public:

    CPU(sc_core::sc_module_name name, Request* instructions, std::uint64_t numInstructions) : sc_module{name},
                                                                                              instructions{instructions},
                                                                                              numInstructions{numInstructions} {
        SC_THREAD(handleInstruction);
        sensitive << clock.pos();

        SC_THREAD(readInstruction);
        sensitive << clock.pos();

        SC_THREAD(countCycles);
        sensitive << clock.neg();
    }

    constexpr std::uint64_t getElapsedCycleCount() const noexcept { return lastCycleWhereWorkWasDone; };

private:
    void handleInstruction() {
        while (true) {
            wait();

            if (instructionReady) {
                instructionReady = false;

                Request currentRequest = instrBus.read();
                addressBus.write(currentRequest.addr);
                dataOutBus.write(currentRequest.data);
                weBus.write(currentRequest.we);

                validDataRequestBus.write(true);

                triggerNextInstructionRead.notify();

                waitForInstructionProcessing();

                lastCycleWhereWorkWasDone = currCycle;

                if (!currentRequest.we) {
                    instructions[program_counter - 1].data = dataInBus;
                }

                validDataRequestBus.write(false);

                if (program_counter == numInstructions) {
                    sc_core::sc_stop();
                }
            }
        }
    }

    void readInstruction() {
        while (true) {
            pcBus.write(program_counter);

            validInstrRequestBus.write(true);
            waitForInstruction();
            validInstrRequestBus.write(false);

            instructionReady = true;

            wait(triggerNextInstructionRead);
            ++program_counter;
        }
    }

    void countCycles() {
        while (true) {
            wait();
            ++currCycle;
        }
    }

    void waitForInstruction() {
        do {
            wait();
        } while (!instrReadyBus);
    }

    void waitForInstructionProcessing() {
        do {
            wait();
        } while (!dataReadyBus);
    }
};