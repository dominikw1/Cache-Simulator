#pragma once

#include "../Request.h"
#include <cstdint>
#include <systemc>
#include <vector>

SC_MODULE(CPU) {
  public:
    // ====================================== External Ports  ======================================
    // Global Clock
    sc_core::sc_in<bool> SC_NAMED(clock);

    // CPU -> Cache
    sc_core::sc_out<std::uint32_t> SC_NAMED(addressBus);
    sc_core::sc_out<std::uint32_t> SC_NAMED(dataOutBus);
    sc_core::sc_out<bool> SC_NAMED(weBus);
    sc_core::sc_out<bool> SC_NAMED(validDataRequestBus);

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
    Request* instructions;

    std::uint64_t program_counter = 0;
    std::uint64_t lastCycleWhereWorkWasDone = 0;

    // needed to handle a parallel instruction read and instruction processing
    bool instructionReady = false;
    sc_core::sc_event triggerNextInstructionRead;

    // this is for when we have just waited for an instruction to be done so we don't need to wait any longer
    bool skipAhead = false;

  public:
    CPU(sc_core::sc_module_name name, Request * instructions);

    /**
     * Returns the cycle in which the last instruction was completed
     * @return cycle in which the last instruction was completed
     */
    constexpr std::uint64_t getElapsedCycleCount() const noexcept { return lastCycleWhereWorkWasDone; }

  private:
    SC_CTOR(CPU); // private since this is never to be called, just to get systemc typedef

    // ========== Main Handling ==============
    /**
     * The main point of the CPU, which sleeps until we received a instruction and then sets all signals needed
     * for the data cache to process the request. In case of a read the data returned from the cache is set to the data
     * field of the instruction.
     */
    void handleInstruction() noexcept;
    /**
     * For the first instruction this sets all needed signals for the instruction cache to read the next instruction.
     * Afterwards the event triggerNextInstructionRead has to be notified to read the next instruction.
     */
    void readInstruction() noexcept;

    // ====================================== Waiting Helpers ======================================
    /**
     * Sleeps until we get a ready signal from instruction cache
     */
    void waitForInstruction() noexcept;
    /**
     * Sleeps until we get a ready signal from data cache
     */
    void waitForInstructionProcessing() noexcept;
};