#include "Request.h"
#include <cstdint>
#include <systemc>
#include <vector>

SC_MODULE(CPU) {
public:
    // CPU -> Cache
    sc_core::sc_out<bool> weBus;
    sc_core::sc_out<bool> validDataRequestBus;
    sc_core::sc_out<std::uint32_t> addressBus;
    sc_core::sc_out<std::uint32_t> dataOutBus;

    // Cache -> CPU
    sc_core::sc_in<std::uint32_t> dataInBus;
    sc_core::sc_in<bool> dataReadyBus;

    // Instr Cache -> CPU
    sc_core::sc_in<Request> instrBus;
    sc_core::sc_in<bool> instrReadyBus;

    // CPU -> Instr Cache
    sc_core::sc_out<bool> validInstrRequestBus;
    sc_core::sc_out<std::uint32_t> pcBus;

    sc_core::sc_in<bool> clock;

#ifdef CPU_DEBUG
    std::vector<std::pair<std::uint32_t, std::uint32_t>> results;
#endif

private:
    std::uint64_t program_counter = 0;
    Request currInstruction;

    std::uint64_t lastCycleWhereWorkWasDone = 0;
    std::uint64_t currCycle = 0;

    bool instructionReady = false;
    sc_core::sc_event triggerNextInstructionRead;

    const std::uint64_t numInstructions;

    SC_CTOR(CPU);

public:

    CPU(sc_core::sc_module_name name, std::uint64_t numInstructions) : sc_module{name},
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

            std::cout << "Cycle:" << currCycle << std::endl;
            if (instructionReady) {
                instructionReady = false;

                std::cout << "Got instruciton at " << currCycle << std::endl;

                Request currentRequest = instrBus.read();
                addressBus.write(currentRequest.addr);
                dataOutBus.write(currentRequest.data);
                weBus.write(currentRequest.we);

                validDataRequestBus.write(true);

                triggerNextInstructionRead.notify();

#ifdef CPU_DEBUG
                results.push_back(std::make_pair(currentRequest.addr, currentRequest.data));
#endif

                waitForInstructionProcessing();
                //ok     std::cout << "Got result at " << currCycle << std::endl;

                //            std::cout << "Receiving data" << std::endl;
                if (currInstruction.we) {
                    // std::cout << "Successfully wrote " << currInstruction.data << " to location " << currInstruction.addr
                    //         << std::endl;
                } else {
                    // std::cout << "Successfully read " << dataInBus.read() << " from address " << currInstruction.addr
                    //         << std::endl;

                }

                lastCycleWhereWorkWasDone = currCycle;

                validDataRequestBus.write(false);

                if (program_counter == numInstructions + 1) {
                    sc_core::sc_stop();
                }
            }
        }
    }

    void readInstruction() {
        while (true) {
            std::cout << "Requesting instruction " << program_counter << " at cycle " << currCycle << std::endl;

            pcBus.write(program_counter);

            validInstrRequestBus.write(true);
            waitForInstruction();
            validInstrRequestBus.write(false);

            instructionReady = true;
            ++program_counter;

            wait(triggerNextInstructionRead);
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