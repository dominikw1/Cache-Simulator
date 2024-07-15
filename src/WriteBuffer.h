#pragma once
#include "OrGate.h"
#include "RingQueue.h"
#include <mutex>
#include <systemc>

struct WriteBufferEntry {
    std::uint32_t address;
    std::uint32_t data;
};

template <std::uint8_t SIZE> SC_MODULE(WriteBuffer) {
  public:
    sc_core::sc_in<bool> clock;

    // Buffer -> Cache
    sc_core::sc_out<bool> SC_NAMED(ready);
    sc_core::sc_out<sc_dt::sc_bv<128>> SC_NAMED(cacheDataOutBus);

    // Cache -> Buffer
    sc_core::sc_in<std::uint32_t> SC_NAMED(cacheAddrBus);
    sc_core::sc_in<std::uint32_t> SC_NAMED(cacheDataInBus);
    sc_core::sc_in<bool> SC_NAMED(cacheWeBus);
    sc_core::sc_in<bool> SC_NAMED(cacheValidRequest);

    // Buffer -> RAM
    sc_core::sc_out<std::uint32_t> SC_NAMED(memoryAddrBus);
    sc_core::sc_out<std::uint32_t> SC_NAMED(memoryDataOutBus);
    sc_core::sc_out<bool> SC_NAMED(memoryWeBus);
    sc_core::sc_out<bool> SC_NAMED(memoryValidRequestBus);

    // RAM -> Buffer
    sc_core::sc_in<sc_dt::sc_bv<128>> SC_NAMED(memoryDataInBus);
    sc_core::sc_in<bool> SC_NAMED(memoryReadyBus);

    // Internal Signals
    sc_core::sc_signal<bool> writerThreadValidMemoryRequest;
    sc_core::sc_signal<bool> readerThreadValidMemoryRequest;
    OrGate SC_NAMED(validMemoryRequestOr);

    WriteBuffer(sc_core::sc_module_name name, std::uint32_t readsPerCacheline)
        : sc_module{name}, readsPerCacheline{readsPerCacheline} {
        using namespace sc_core;

        validMemoryRequestOr.a.bind(writerThreadValidMemoryRequest);
        validMemoryRequestOr.b.bind(readerThreadValidMemoryRequest);
        validMemoryRequestOr.out.bind(memoryValidRequestBus);

        SC_THREAD(handleIncomingRequests);
        SC_THREAD(doWrites);
    }

  private:
    SC_CTOR(WriteBuffer);

    RingQueue<WriteBufferEntry> buffer{SIZE};
    std::mutex currWritingMarkerLock;
    bool currWritingMarker = false;

    void setCurrRequest(bool newValue) {
        std::lock_guard<std::mutex> g{currWritingMarkerLock};
        currWritingMarker = newValue;
    }

    bool currentlyWriting() {
        std::lock_guard<std::mutex> g{currWritingMarkerLock};
        return currWritingMarker;
    }

    void setCurrWriting(bool newValue) {
        std::lock_guard<std::mutex> g{currWritingMarkerLock};
        currWritingMarker = newValue;
    }

    void writeToRAM() {
        assert(buffer.getSize() > 0);
        WriteBufferEntry next = buffer.pop();
        memoryAddrBus.write(next.address);
        memoryDataOutBus.write(next.data);
        memoryWeBus.write(true);
        writerThreadValidMemoryRequest.write(true);
        std::cout << "Waiting for RAM write " << std::endl;
        do {
            wait(clock.posedge_event());
        } while ((!memoryReadyBus.read()));
        std::cout << "Done waiting for RAM write " << std::endl;
        writerThreadValidMemoryRequest.write(false);
     //   wait(clock.posedge_event());
    }

    const std::uint32_t readsPerCacheline;

    void passReadAlong() {
        memoryAddrBus.write(cacheAddrBus.read());
        memoryWeBus.write(false);
        readerThreadValidMemoryRequest.write(true);

        std::cout << "Now waiting for RAM" << std::endl;
        do {
            wait(clock.posedge_event());
        } while (!memoryReadyBus.read());
        readerThreadValidMemoryRequest.write(false);
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
                if (!cacheWeBus.read() && buffer.getSize() == 0 && !currentlyWriting()) {
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
                setCurrWriting(true);
                writeToRAM();
                setCurrWriting(false);
            }
        }
    }
};
