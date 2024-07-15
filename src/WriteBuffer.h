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
        sensitive << clock.pos();
        SC_THREAD(doWrites);
        sensitive << clock.pos();
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

    void relinquishWriting() {
        std::lock_guard<std::mutex> g{currWritingMarkerLock};
        currWritingMarker = false;
    }

    bool tryAcquireWriting() {
        std::lock_guard<std::mutex> g{currWritingMarkerLock};
        if (!currWritingMarker) {
            currWritingMarker = true;
            return true;
        } else {
            return false;
        }
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
            wait();
        } while ((!memoryReadyBus.read()));
        std::cout << "Done waiting for RAM write " << std::endl;
        writerThreadValidMemoryRequest.write(false);
        wait();
    }

    const std::uint32_t readsPerCacheline;

    void passReadAlong() {
        memoryAddrBus.write(cacheAddrBus.read());
        memoryWeBus.write(false);
        readerThreadValidMemoryRequest.write(true);

        std::cout << "Now waiting for RAM" << std::endl;
        do {
            wait();
        } while (!memoryReadyBus.read());
        readerThreadValidMemoryRequest.write(false);
        std::cout << " Done waiting for RAM" << std::endl;

        ready.write(true);
        // dont need to wait before first one because we can only get here if RAM tells us it is ready
        for (int i = 0; i < readsPerCacheline; ++i) {
            cacheDataOutBus.write(memoryDataInBus.read());
            wait();
        }

        ready.write(false);
        wait();
    }

    void forceBufferFlush(bool& holdingLock) {
        while (buffer.getSize() > 0) {
            std::cout << "FORCE Entering actual write" << std::endl;
            if (tryAcquireWriting()) {
                writeToRAM();
                holdingLock = true;
            }
        }
    }

    void handleIncomingRequests() {
        bool holdingWriteLock = false;
        while (true) {
            wait();
            if (holdingWriteLock) {
                relinquishWriting();
                holdingWriteLock = false;
            }
            if (cacheValidRequest.read()) {
                // std::cout << "WB: found valid instr request" << std::endl;
                // std::cout << "State: " << cacheWeBus.read() << " " << buffer.getSize() << " " <<
                // currentlyWriting()
                //        << std::endl;
                ready.write(false);
                if (!cacheWeBus.read() && buffer.getSize() == 0 && !currentlyWriting()) {
                    // can only handle if all writes are through for sequential consistency reasons
                    std::cout << "Passing along read " << std::endl;
                    passReadAlong();
                } else {
                    if (cacheWeBus.read() && buffer.getSize() != buffer.getCapacity()) {
                        // can only handle if there is space in buffer
                        buffer.push(WriteBufferEntry{cacheAddrBus.read(), cacheDataInBus.read()});
                        ready.write(true);
                        wait();
                    } else {
                        if (!cacheWeBus.read() && !currentlyWriting()) {
                            forceBufferFlush(holdingWriteLock);
                        }
                    }
                }
            }
        }
    }

    void doWrites() {
        bool holdingWriteLock = false;
        while (true) {
            wait();
            if (holdingWriteLock) {
                relinquishWriting();
                holdingWriteLock = false;
            }
            //   std::cout << "Entering write loop " << std::endl;
            if ((cacheValidRequest.read() && cacheWeBus.read()) || currentlyWriting()) {
                continue;
            }
            //  std::cout << "Trying to write" << std::endl;
            if (buffer.getSize() > 0) {
                std::cout << "Entering actual write" << std::endl;
                if (tryAcquireWriting()) {
                    writeToRAM();
                    holdingWriteLock = true;
                }
                std::cout << "Ending actual write" << std::endl;
            }
        }
    }
};
