#pragma once
#include "RingQueue.h"
#include <cassert>
#include <cstdint>
#include <systemc>
/**
 * This module represents a buffer for the cache. All RAM-memory accesses go through this buffer.
 *
 * Its main purpose is, as its name says, to buffer writes. Since the cache usually does not care about whether his
 * write has already arrived in RAM or that it not having done so yet will simply be guaranteed to not interfere with
 * its future operations, this can save us a lot of cycles.
 *
 * Its default behaviour is to buffer all writes and, when a read happens, to check for whether the read would be
 * affected by buffered writes. If that is not the case, the read gets performed there and then, before the remaining
 * writes. As this is not completely sequentially consistent (imagine a peripheral that keeps track of reads happening
 * on it and therefore possibly changing what we would read on our read), there is the option of compiling this cache
 * with the definition STRICT_RAM_READ_AFTER_WRITES. This will lead to the behaviour of reads only going through if all
 * writes are done.
 *
 * Operations happen on both rising and falling edge. Since the write buffer is a very small component, having only half
 * a cycle for both kinds of operations is fine.
 * On rising edge: The state is updated depending on current state and external busses
 * On falling edge: An operation is performed depending on which state we decided to be in. This operation is either a
 * read or a write (or nothing)
 */

template <std::uint8_t SIZE> SC_MODULE(WriteBuffer) {
  public:
    // Global -> Buffer
    sc_core::sc_in<bool> SC_NAMED(clock);

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

    const std::uint32_t readsPerCacheline;
    const std::uint32_t cacheLineSize;

    WriteBuffer(sc_core::sc_module_name name, std::uint32_t readsPerCacheline, std::uint32_t cacheLineSize) noexcept
        : sc_module{name}, readsPerCacheline{readsPerCacheline}, cacheLineSize{cacheLineSize} {
        using namespace sc_core;

        SC_THREAD(updateState);
        sensitive << clock.pos();
        SC_THREAD(handleWrite);
        sensitive << clock.neg();
        SC_THREAD(handleRead);
        sensitive << clock.neg();
    }

  private:
    SC_CTOR(WriteBuffer);

    struct WriteBufferEntry {
        std::uint32_t address;
        std::uint32_t data;
    };

    enum class State {
        Write,
        Read,
        Idle,
    };

    RingQueue<WriteBufferEntry> buffer{SIZE};
    State state = State::Idle;
    bool pending = false;

    // ============= State update =============
    void updateState() noexcept;
    void acceptWriteRequest() noexcept;
    void acceptReadRequest() noexcept;

    // ============= Reading =============
    void handleRead() noexcept;
    void passReadAlong() noexcept;

    // ============= Writing =============
    void handleWrite() noexcept;
    void writeToRAM() noexcept;

    // ============= Helpers =============
    constexpr bool weCanAcceptWrite() noexcept;
    constexpr bool weCanAcceptRead() noexcept;
    bool shouldStartNextWrite() noexcept;
    bool thereIsAWrite() noexcept;
    bool thereIsARead() noexcept;
    bool isReadAddrInWriteBuffer(std::uint32_t readAddr) noexcept;
    constexpr std::uint32_t makeAddrAligned(std::uint32_t addr) noexcept;
};

template <std::uint8_t SIZE> void WriteBuffer<SIZE>::writeToRAM() noexcept {
    assert(buffer.getSize() > 0);
    WriteBufferEntry next = buffer.pop();
    memoryAddrBus.write(next.address);
    memoryDataOutBus.write(next.data);
    memoryWeBus.write(true);
    memoryValidRequestBus.write(true);
    while (!memoryReadyBus.read()) {
        wait();
    }
    memoryValidRequestBus.write(false);
}

template <std::uint8_t SIZE> void WriteBuffer<SIZE>::passReadAlong() noexcept {
    memoryAddrBus.write(cacheAddrBus.read());
    memoryWeBus.write(false);
    memoryValidRequestBus.write(true);

    while (!memoryReadyBus.read()) {
        wait();
    }
    memoryValidRequestBus.write(false);

    ready.write(true);
    // dont need to wait before first one because we can only get here if RAM tells us it is ready
    for (std::size_t i = 0; i < readsPerCacheline; ++i) {
        cacheDataOutBus.write(memoryDataInBus.read());
        wait();
    }

    ready.write(false);
    wait(); // this wait is needed because otherwise on rising edge this would instantly be overriden ig??
}

template <std::uint8_t SIZE> constexpr std::uint32_t WriteBuffer<SIZE>::makeAddrAligned(std::uint32_t addr) noexcept {
    return (addr / cacheLineSize) * cacheLineSize;
}

template <std::uint8_t SIZE> bool WriteBuffer<SIZE>::isReadAddrInWriteBuffer(std::uint32_t readAddr) noexcept {
    return buffer.any([readAddr, this](WriteBufferEntry& entry) { return makeAddrAligned(entry.address) == readAddr; });
}

template <std::uint8_t SIZE> constexpr bool WriteBuffer<SIZE>::weCanAcceptWrite() noexcept {
    return state == State::Idle || state == State::Write;
}

template <std::uint8_t SIZE> constexpr bool WriteBuffer<SIZE>::weCanAcceptRead() noexcept {
#ifdef STRICT_RAM_READ_AFTER_WRITES
    return state == State::Idle && buffer.getSize() == 0;
#else
    return state == State::Idle;
#endif
}

template <std::uint8_t SIZE> bool WriteBuffer<SIZE>::thereIsAWrite() noexcept {
    return (cacheValidRequest.read() && cacheWeBus.read()) || pending;
}
template <std::uint8_t SIZE> bool WriteBuffer<SIZE>::thereIsARead() noexcept {
    return cacheValidRequest.read() && !cacheWeBus.read();
}

template <std::uint8_t SIZE> void WriteBuffer<SIZE>::acceptWriteRequest() noexcept {
    if (buffer.getSize() < SIZE) {
        buffer.push(WriteBufferEntry{cacheAddrBus.read(), cacheDataInBus.read()});
        ready.write(true);
        state = State::Write;
        pending = false;
    } else {
        ready.write(false);
        state = State::Write;
        pending = true;
    }
}

template <std::uint8_t SIZE> void WriteBuffer<SIZE>::acceptReadRequest() noexcept {
    // do read immediately unless the tag is in the buffer
    if (isReadAddrInWriteBuffer(cacheAddrBus.read())) {
        state = State::Write;
    } else {
        state = State::Read;
    }
}

template <std::uint8_t SIZE> bool WriteBuffer<SIZE>::shouldStartNextWrite() noexcept {
    return state == State::Idle && buffer.getSize() > 0;
}

template <std::uint8_t SIZE> void WriteBuffer<SIZE>::updateState() noexcept {
    while (true) {
        wait();
        if (state != State::Read) // only in read is it possible we are actually ready rn
            ready.write(false);   // this is a kind of catch-all safeguard that we aren't falsely reporting
                                  // readiness. Should never be an issue though

        if (weCanAcceptWrite() && thereIsAWrite()) {
            acceptWriteRequest();
            continue; // we can short - circuit here. We already know what to do next
        }

        if (weCanAcceptRead() && thereIsARead()) {
            acceptReadRequest();
            continue; // we can short - circuit here. We already know what to do next
        }

        if (shouldStartNextWrite()) {
            state = State::Write;
        }
    }
}

template <std::uint8_t SIZE> void WriteBuffer<SIZE>::handleRead() noexcept {
    while (true) {
        wait();
        if (state == State::Read) {
            passReadAlong();
            state = State::Idle;
        }
    }
}

template <std::uint8_t SIZE> void WriteBuffer<SIZE>::handleWrite() noexcept {
    while (true) {
        wait();
        if (state == State::Write) {
            writeToRAM();
            state = State::Idle;
        }
    }
}