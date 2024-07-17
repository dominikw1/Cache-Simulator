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
    sc_core::sc_event instructionCycleDone;

    std::uint64_t lastCycleWhereWorkWasDone = 0;

  public:
    SC_CTOR(CPU) {
        SC_THREAD(handleInstruction);
        sensitive << clock.pos();
        dont_initialize();
    }

    constexpr std::uint64_t getElapsedCycleCount() const noexcept { return lastCycleWhereWorkWasDone; };

  private:
    void handleInstruction() {
        while (true) {
            wait(clock.posedge_event());

            std::cout << "Requesting instruction " << program_counter << std::endl;
            pcBus.write(program_counter);
            validInstrRequestBus.write(true);

            waitForInstruction();

            validInstrRequestBus.write(false);

            Request currentRequest = instrBus.read();
            addressBus.write(currentRequest.addr);
            dataOutBus.write(currentRequest.data);
            weBus.write(currentRequest.we);
            validDataRequestBus.write(true);

            waitForInstructionProcessing();

            std::cout << "Receiving data" << std::endl;
            if (currInstruction.we) {
                std::cout << "Successfully wrote " << currInstruction.data << " to location " << currInstruction.addr
                          << std::endl;
            } else {
                std::cout << "Successfully read " << dataInBus.read() << " from address " << currInstruction.addr
                          << std::endl;
#ifdef CPU_DEBUG
                results.push_back(std::make_pair(currentRequest.addr, currentRequest.data));
#endif
            }

            lastCycleWhereWorkWasDone = sc_core::sc_time_stamp().value() / 1000;

            validDataRequestBus.write(false);
            ++program_counter;
        }
    }

    void waitForInstruction() {
        do {
            wait(clock.posedge_event());
        } while (!instrReadyBus);
    }

    void waitForInstructionProcessing() {
        do {
            wait(clock.posedge_event());
        } while (!dataReadyBus);
    }
};