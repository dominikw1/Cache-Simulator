#pragma once
#include "RingQueue.h"
#include <mutex>
#include <systemc>

struct WriteBufferEntry {
    std::uint32_t address;
    std::uint32_t data;
};

// at runtime übergene!!
template <std::uint8_t SIZE> SC_MODULE(WriteBuffer) {
  public:
    sc_core::sc_in<bool> clock;

    // Buffer -> Cache
    sc_core::sc_out<bool> ready{"readyBus"};
    sc_core::sc_out<sc_dt::sc_bv<128>> cacheDataOutBus{"cpuDataOutBus"};

    // Cache -> Buffer
    sc_core::sc_in<std::uint32_t> cacheAddrBus{"cacheAddrBus"};
    sc_core::sc_in<std::uint32_t> cacheDataInBus{"cacheDataInBus"};
    sc_core::sc_in<bool> cacheWeBus{"cacheWeBus"};
    sc_core::sc_in<bool> cacheValidRequest{"cacheValidRequest"};

    // Buffer -> RAM
    sc_core::sc_out<std::uint32_t> memoryAddrBus{"memoryAddrBus"};
    sc_core::sc_out<std::uint32_t> memoryDataOutBus{"memoryDataOutBus"};
    sc_core::sc_out<bool> memoryWeBus{"memoryWeBus"};
    sc_core::sc_out<bool> memoryValidRequestBus{"memoryValidRequestBus"};

    // RAM -> Buffer
    sc_core::sc_in<sc_dt::sc_bv<128>> memoryDataInBus{"memoryDataInBus"};
    sc_core::sc_in<bool> memoryReadyBus{"memoryReadyBus"};

    WriteBuffer(sc_core::sc_module_name name, std::uint32_t readsPerCacheline)
        : sc_module{name}, readsPerCacheline{readsPerCacheline} {
        using namespace sc_core;
        SC_THREAD(handleIncomingRequests);
        // sensitive << clock.pos();

        SC_THREAD(doWrites);
        // sensitive << clock.pos();
    }

  private:
    SC_CTOR(WriteBuffer);

    RingQueue<WriteBufferEntry> buffer{SIZE};

    void writeToRAM() {
        assert(buffer.getSize() > 0);
        WriteBufferEntry next = buffer.pop();
        memoryAddrBus.write(next.address);
        memoryDataOutBus.write(next.data);
        memoryWeBus.write(true);
        memoryValidRequestBus.write(true);
        std::cout << "Waiting for RAM write " << std::endl;
        do {
            wait(clock.posedge_event());
        } while ((!memoryReadyBus.read()));
        std::cout << "Done waiting for RAM write " << std::endl;
        memoryValidRequestBus.write(false);
    }

    const std::uint32_t readsPerCacheline;

    void passReadAlong() {
        memoryAddrBus.write(cacheAddrBus.read());
        memoryWeBus.write(false);
        memoryValidRequestBus.write(true);

        std::cout << "Now waiting for RAM" << std::endl;
        do {
            wait(clock.posedge_event());
        } while (!memoryReadyBus.read());
        memoryValidRequestBus.write(false);
        std::cout << " Done waiting for RAM" << std::endl;

        ready.write(true);
        // dont need to wait before first one because we can only get here if RAM tells us it is ready
        for (int i = 0; i < readsPerCacheline; ++i) {
            cacheDataOutBus.write(memoryDataOutBus.read());
            wait(clock.posedge_event());
        }

        ready.write(false);
        wait(clock.posedge_event());
    }

    void handleIncomingRequests() {
        while (true) {
            wait(clock.posedge_event());

            if (cacheValidRequest.read()) {
                std::cout << "WB: found valid instr request" << std::endl;
                ready.write(false);
                if (!cacheWeBus.read() && buffer.getSize() == 0) {
                    // can only handle if all writes are through for sequential consistency reasons
                    passReadAlong();
                } else {
                    if (cacheWeBus.read() && buffer.getSize() != buffer.getCapacity()) {
                        // can only handle if there is space in buffer
                        buffer.push(WriteBufferEntry{cacheAddrBus.read(), cacheDataInBus.read()});
                        ready.write(true);
                        wait(clock.posedge_event());
                    }
                }
            }
        }
    }

    void doWrites() {
        while (true) {
            wait(clock.posedge_event());
            // std::unique_lock<std::mutex> lock{mutex};
            if (cacheValidRequest.read()) {
                continue;
            }
            // handledRequestForCycleCV.wait(lock, [this] { return handledRequestForCycle; });

            // if write reqeust: just add it to the buffer

            if (buffer.getSize() > 0) {

                writeToRAM();
            }
        }
    }
};
