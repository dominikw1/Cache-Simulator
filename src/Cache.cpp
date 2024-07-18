#include "Cache.h"

template <>
inline std::vector<Cacheline>::iterator
Cache<MappingType::Direct>::getCachelineOwnedByAddr(DecomposedAddress decomposedAddr) {
    auto cachelineExpectedAt = cacheInternal.begin() + decomposedAddr.index;
    if (cachelineExpectedAt->isUsed && cachelineExpectedAt->tag == decomposedAddr.tag) {
        return cachelineExpectedAt;
    } else {
        return cacheInternal.end();
    }
}

template <>
inline std::vector<Cacheline>::iterator
Cache<MappingType::Fully_Associative>::getCachelineOwnedByAddr(DecomposedAddress decomposedAddr) {
    return std::find_if(cacheInternal.begin(), cacheInternal.end(), [&decomposedAddr](Cacheline& cacheline) {
        return cacheline.isUsed && cacheline.tag == decomposedAddr.tag;
    });
}

template <>
inline std::vector<Cacheline>::iterator
Cache<MappingType::Direct>::chooseWhichCachelineToFillFromRAM(DecomposedAddress decomposedAddr) {
    auto cachelineToWriteInto = cacheInternal.begin() + decomposedAddr.index; // there is only one possible space
    assert(cachelineToWriteInto != cacheInternal.end());
    return cachelineToWriteInto;
}

template <>
inline std::vector<Cacheline>::iterator Cache<MappingType::Fully_Associative>::chooseWhichCachelineToFillFromRAM(
    __attribute__((unused)) DecomposedAddress decomposedAddr) {
    auto firstUnusedCacheline = std::find_if(cacheInternal.begin(), cacheInternal.end(),
                                             [](const Cacheline& cacheline) { return !cacheline.isUsed; });
    if (firstUnusedCacheline == cacheInternal.end()) {
        firstUnusedCacheline = cacheInternal.begin() + replacementPolicy->pop();
    }
    assert(firstUnusedCacheline != cacheInternal.end());
    return firstUnusedCacheline;
}

template <>
inline constexpr DecomposedAddress Cache<MappingType::Direct>::decomposeAddress(std::uint32_t address) noexcept {
    assert(addressOffsetBitMask > 0 && addressIndexBitMask > 0 && addressTagBitMask > 0);
    return DecomposedAddress{((address >> addressOffsetBits) >> addressIndexBits) & addressTagBitMask,
                             (address >> addressOffsetBits) & addressIndexBitMask, address & addressOffsetBitMask};
}

template <>
inline constexpr DecomposedAddress
Cache<MappingType::Fully_Associative>::decomposeAddress(std::uint32_t address) noexcept {
    assert(addressOffsetBitMask > 0 && addressIndexBitMask == 0 && addressTagBitMask > 0);
    return DecomposedAddress{(address >> addressOffsetBits) & addressTagBitMask, 0, address & addressOffsetBitMask};
}

template <>
inline void
Cache<MappingType::Direct>::registerUsage(__attribute__((unused)) std::vector<Cacheline>::iterator cacheline) {
    // no bookeeping needed
}

template <>
inline void Cache<MappingType::Fully_Associative>::registerUsage(std::vector<Cacheline>::iterator cacheline) {
    replacementPolicy->logUse(cacheline - cacheInternal.begin());
}

template <MappingType mappingType> inline void Cache<mappingType>::waitForRAM() {
    //  std::cout << "Now waiting for WriteBuffer" << std::endl;
    do {
        wait(clock.posedge_event());
    } while (!writeBufferReady.read());

    // std::cout << " Done waiting for WriteBuffer" << std::endl;
}

template <> constexpr inline void Cache<MappingType::Fully_Associative>::precomputeAddressDecompositionBits() noexcept {
    addressOffsetBits = safeCeilLog2(cacheLineSize);
    addressIndexBits = 0; // no index bits in fully associative cache
    addressTagBits = 32 - addressOffsetBits;

    addressOffsetBitMask = generateBitmaskForLowestNBits(addressOffsetBits);
    addressTagBitMask = generateBitmaskForLowestNBits(addressTagBits);
}

template <> constexpr inline void Cache<MappingType::Direct>::precomputeAddressDecompositionBits() noexcept {
    addressOffsetBits = safeCeilLog2(cacheLineSize);
    addressIndexBits = safeCeilLog2(numCacheLines);
    addressTagBits = 32 - addressIndexBits - addressOffsetBits;

    addressOffsetBitMask = generateBitmaskForLowestNBits(addressOffsetBits);
    addressIndexBitMask = generateBitmaskForLowestNBits(addressIndexBits);
    addressTagBitMask = generateBitmaskForLowestNBits(addressTagBits);
}

template <MappingType mappingType> inline void Cache<mappingType>::setUpWriteBufferConnects() {
    writeBuffer.clock.bind(clock);

    writeBuffer.ready.bind(writeBufferReady);
    writeBuffer.cacheDataOutBus.bind(writeBufferDataOut);

    writeBuffer.cacheAddrBus.bind(writeBufferAddr);
    writeBuffer.cacheDataInBus.bind(writeBufferDataIn);
    writeBuffer.cacheWeBus.bind(writeBufferWE);
    writeBuffer.cacheValidRequest.bind(writeBufferValidRequest);

    writeBuffer.memoryAddrBus.bind(memoryAddrBus);
    writeBuffer.memoryDataOutBus.bind(memoryDataOutBus);
    writeBuffer.memoryWeBus.bind(memoryWeBus);
    writeBuffer.memoryValidRequestBus.bind(memoryValidRequestBus);

    writeBuffer.memoryDataInBus.bind(memoryDataInBus);
    writeBuffer.memoryReadyBus.bind(memoryReadyBus);
}

template <MappingType mappingType> inline void Cache<mappingType>::zeroInitialiseCachelines() {
    for (auto& cacheline : cacheInternal) {
        cacheline.data = std::vector<std::uint8_t>(cacheLineSize, 0);
    }
}

template <MappingType mappingType>
inline Cache<mappingType>::Cache(sc_core::sc_module_name name, unsigned int numCacheLines, unsigned int cacheLineSize,
                                 unsigned int cacheLatency, std::unique_ptr<ReplacementPolicy<std::uint32_t>> policy)
    : sc_module{name}, numCacheLines{numCacheLines}, cacheLineSize{cacheLineSize}, cacheLatency{cacheLatency},
      replacementPolicy{std::move(policy)}, cacheInternal{numCacheLines},
      writeBuffer{"writeBuffer", cacheLineSize / RAM_READ_BUS_SIZE_IN_BYTE, cacheLineSize} {

    using namespace sc_core; // in scope as to not pollute global namespace
    if (replacementPolicy != nullptr && mappingType == MappingType::Direct) {
        std::cout << "Replacement Policy is set on a direct mapped cache - this has no effect.\n";
    }

    zeroInitialiseCachelines();
    precomputeAddressDecompositionBits();
    setUpWriteBufferConnects();

    SC_THREAD(handleRequest);
    sensitive << clock.pos();
}

template <MappingType mappingType>
inline std::vector<Cacheline>::iterator
Cache<mappingType>::writeRAMReadIntoCacheline(DecomposedAddress decomposedAddr) {
    //  std::cout << "Writing RAM Read into cacheline " << std::endl;
    auto cachelineToWriteInto = chooseWhichCachelineToFillFromRAM(decomposedAddr);

    // we do not allow any inputs violating this rule
    assert(cachelineToWriteInto->data.size() % RAM_READ_BUS_SIZE_IN_BYTE == 0);
    sc_dt::sc_bv<RAM_READ_BUS_SIZE_IN_BYTE * BITS_IN_BYTE> dataRead;
    std::uint32_t numReadEvents = (cachelineToWriteInto->data.size() / RAM_READ_BUS_SIZE_IN_BYTE);

    // dont need to wait before first one because we can only get here if RAM tells us it is ready
    for (std::size_t i = 0; i < numReadEvents; ++i) {
        dataRead = writeBufferDataOut.read();
        //    std::cout << "Cache received: " << writeBufferDataOut.read() << std::endl;

        for (int byte = 0; byte < RAM_READ_BUS_SIZE_IN_BYTE; ++byte) {
            cachelineToWriteInto->data[RAM_READ_BUS_SIZE_IN_BYTE * i + byte] =
                dataRead.range(BITS_IN_BYTE * byte + (BITS_IN_BYTE - 1), BITS_IN_BYTE * byte).to_uint();
            //   std::cout << dataRead.range(BITS_IN_BYTE * byte + (BITS_IN_BYTE - 1), BITS_IN_BYTE * byte).to_uint() <<
            //   "/"
            //           << cachelineToWriteInto->data[RAM_READ_BUS_SIZE_IN_BYTE * i + byte] << " ";
        }
        // std::cout << std::endl;
        //  if this is the last one we don't need to wait anymore
        if (i + 1 <= numReadEvents)
            wait(clock.posedge_event());
    }
//    std::cout << "Cache: Done reading into cacheline at " << sc_core::sc_time_stamp() << "\n";

    writeBufferValidRequest.write(false);
    // wait(clock.posedge_event()); // DEBUG
    // wait(clock.posedge_event()); // DEBUG
    // std::cout << "Unsetting valid request now" << std::endl;
    cachelineToWriteInto->isUsed = true;
    cachelineToWriteInto->tag = decomposedAddr.tag;

    // std::cout << "Done writing RAM Read into cacheline :" << *cachelineToWriteInto << std::endl;

    return cachelineToWriteInto;
}

template <MappingType mappingType> inline void Cache<mappingType>::waitOutCacheLatency() {
    for (std::size_t i = 0; i < cacheLatency; ++i) {
        wait();
    }
}

template <MappingType mappingType>
inline std::vector<Cacheline>::iterator Cache<mappingType>::fetchIfNotPresent(std::uint32_t addr,
                                                                              DecomposedAddress decomposedAddr) {
    waitOutCacheLatency();

    auto cacheline = getCachelineOwnedByAddr(decomposedAddr);
    if (cacheline != cacheInternal.end()) {
        // std::cout << "HIT" << std::endl;
        ++hitCount;
        return cacheline;
    }
    // std::cout << "MISS" << std::endl;
    ++missCount;

    startReadFromRAM(addr);
    waitForRAM();
    return writeRAMReadIntoCacheline(decomposedAddr);
}

template <MappingType mappingType>
inline void Cache<mappingType>::handleSubRequest(SubRequest subRequest, std::uint32_t& readData) {
    // std::cout << "Starting handling subrequest " << subRequest.addr << " " << subRequest.data << " "
    //         << unsigned(subRequest.size) << " " << subRequest.we << " " << unsigned(subRequest.bitsBefore)
    //       << std::endl;

    // std::cout << "Potentially reading into cache if not present...\n";
    auto addr = subRequest.addr;

    // split into tag - index - offset
    auto decomposedAddr = decomposeAddress(addr);
    // check if in cache (waits for #cycles specified by cacheLatency) - if not read from RAM

    // std::cout << "Curr tag: " << decomposedAddr.tag << std::endl;
    // std::cout << "Curr index " << decomposedAddr.index << " offset " << decomposedAddr.offset << std::endl;
    //  std::cout << "Tags in cache:" << std::endl;
    // for (auto& cachl : cacheInternal) {
    //   std::cout << cachl.tag << "\n";
    //}

    auto cacheline = fetchIfNotPresent(addr, decomposedAddr);

    // std::cout << "Done reading into cache\n";

    // if a fully associative Cache and we use a stateful policy we declare the use of the cacheline here. In Direct
    // Mapped cache this is a NOP
    registerUsage(cacheline);
    // std::cout << "Cacheline we read looks like: " << *cacheline << std::endl;

    if (subRequest.we) {
        doWrite(*cacheline, decomposedAddr, subRequest.data, subRequest.size);
        passWriteOnToRAM(*cacheline, decomposedAddr, addr);
        // std::cout << "After writing it now looks like: " << *cacheline << std::endl;
    } else {
        auto tempReadData = doRead(decomposedAddr, *cacheline, subRequest.size);
        // std::cout << "Just read " << tempReadData << " from cache\n";
        readData = applyPartialRead(subRequest, readData, tempReadData);
    }

    // std::cout << "Done with subcycle" << std::endl;
}

template <MappingType mappingType>
inline std::uint32_t Cache<mappingType>::doRead(DecomposedAddress decomposedAddr, Cacheline& cacheline,
                                                std::uint8_t numBytes) {
    //  std::cout << unsigned(numBytes) << " " << decomposedAddr.offset << std::endl;
    assert((numBytes + decomposedAddr.offset - 1) < cacheLineSize);

    std::uint32_t retVal = 0;
    for (std::size_t byteNr = 0; byteNr < numBytes; ++byteNr) {
        retVal += cacheline.data.at(decomposedAddr.offset + byteNr) << (byteNr * BITS_IN_BYTE);
    }
    return retVal;
}

template <MappingType mappingType> inline void Cache<mappingType>::handleRequest() {
    while (true) {
        wait();
        ready.write(false);

        if (!cpuValidRequest.read())
            continue;
        //std::cout << "Cache: Got request at " << sc_core::sc_time_stamp() << "\n";

        cyclesPassedInRequest = 0;

        auto request = constructRequestFromBusses();
        //   std::cout << "Received request:\n" << request << "\n";
        auto subRequests = splitRequestIntoSubRequests(request, cacheLineSize);

        // while this is also passed into write requests, it is only relevant for read request and will not be accessed
        // if invalid for the request type
        std::uint32_t readData = 0;

        for (auto& subRequest : subRequests) {
            handleSubRequest(std::move(subRequest), readData);
        }

        if (!request.we) {
            //   std::cout << "Sending " << readData << " back to CPU" << std::endl;
            cpuDataOutBus.write(readData);
        }

        //   std::cout << "Done with cycle" << std::endl;

        // it is the responsibility of the CPU to have stopped the valid request signal at the latest during the cycle
        // he gets this signal. To not still read the valid request signal from the previous cycle we sleep for one and
        // only then start checking again
        ready.write(true);
       // std::cout << sc_core::sc_get_current_process_b()->get_parent()->basename() << ": Done with request at "
         //         << sc_core::sc_time_stamp() << "\n";

        wait(); // can we get rid of this somehow??s
    }
}

template <MappingType mappingType>
inline void Cache<mappingType>::doWrite(Cacheline& cacheline, DecomposedAddress decomposedAddr, std::uint32_t data,
                                        std::uint8_t numBytes) {
    //  std::cout << unsigned(numBytes) << " " << decomposedAddr.offset << std::endl;

    assert((numBytes + decomposedAddr.offset - 1) < cacheLineSize);
    //   std::cout << "Attempting to write " << data << std::endl;
    for (std::size_t byteNr = 0; byteNr < numBytes; ++byteNr) {
        cacheline.data[(decomposedAddr.offset + byteNr)] =
            (data >> BITS_IN_BYTE * byteNr) & generateBitmaskForLowestNBits(BITS_IN_BYTE);
    }
    //  std::cout << "Done writing into cacheline" << std::endl;
}

template <MappingType mappingType>
inline void Cache<mappingType>::passWriteOnToRAM(Cacheline& cacheline, DecomposedAddress decomposedAddr,
                                                 std::uint32_t addr) {
    std::uint32_t data = 0;

    std::size_t startByte = std::min(static_cast<std::size_t>(decomposedAddr.offset), cacheline.data.size() - 4);

    data |= (cacheline.data[startByte + 0]) << 0 * BITS_IN_BYTE;
    data |= (cacheline.data[startByte + 1]) << 1 * BITS_IN_BYTE;
    data |= (cacheline.data[startByte + 2]) << 2 * BITS_IN_BYTE;
    data |= (cacheline.data[startByte + 3]) << 3 * BITS_IN_BYTE;

    // std::cout << "Passing to write buffer" << std::endl;
    writeBufferAddr.write((startByte == static_cast<std::size_t>(decomposedAddr.offset)
                               ? (addr)
                               : ((addr / cacheLineSize) * cacheLineSize + cacheline.data.size() - 4)));
    writeBufferDataIn.write(data);
    writeBufferWE.write(true);
    // wait(clock.posedge_event()); // DEBUG
    // wait(clock.posedge_event()); // DEBUG
    writeBufferValidRequest.write(true);
    // wait(clock.posedge_event()); // DEBUG
    // wait(clock.posedge_event()); // DEBUG
    // write buffer takes at least one cycle (DEFINITION)
    do {
        wait(clock.posedge_event());
        cyclesPassedInRequest++;
    } while (!writeBufferReady);
    // std::cout << "Buffer said was ready" << std::endl;
    writeBufferValidRequest.write(false);
    // wait(clock.posedge_event()); // DEBUG
    // wait(clock.posedge_event()); // DEBUG
    // std::cout << "Passed to write buffer " << std::endl;
}

template <MappingType mappingType> inline void Cache<mappingType>::startReadFromRAM(std::uint32_t addr) {
    std::uint32_t alignedAddr = (addr / cacheLineSize) * cacheLineSize;
    writeBufferAddr.write(alignedAddr);
    writeBufferWE.write(false);
    //  std::cout << "Setting WB valid req to true in read " << std::endl;
    // wait(clock.posedge_event()); // DEBUG
    writeBufferValidRequest.write(true);
}

template <MappingType mappingType> inline Request Cache<mappingType>::constructRequestFromBusses() {
    return Request{cpuAddrBus.read(), cpuDataInBus.read(), cpuWeBus.read()};
}

template class Cache<MappingType::Direct>;
template class Cache<MappingType::Fully_Associative>;