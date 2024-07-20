#include "Cache.h"
#include "Saturating_Arithmetic.h"
using namespace sc_core;

template <>
std::vector<Cacheline>::iterator
Cache<MappingType::Direct>::getCachelineOwnedByAddr(DecomposedAddress decomposedAddr) noexcept {
    auto cachelineExpectedAt = cacheInternal.begin() + decomposedAddr.index;
    if (cachelineExpectedAt->isValid && cachelineExpectedAt->tag == decomposedAddr.tag) {
        return cachelineExpectedAt;
    } else {
        return cacheInternal.end();
    }
}

template <>
std::vector<Cacheline>::iterator
Cache<MappingType::Fully_Associative>::getCachelineOwnedByAddr(DecomposedAddress decomposedAddr) noexcept {
    wait(); // reference hash table implementation has a frequency of 370MHz, we run at 1GHz. To compensate for this
    wait(); // dicreptancy, we wait 2 extra cycles.  (source: https://ar5iv.labs.arxiv.org/html/2108.03390v2)

    if (cachelineLookupTable.count(decomposedAddr.tag)) {
        // isValid is true by virtue of the tag being in there
        return cacheInternal.begin() + cachelineLookupTable[decomposedAddr.tag];
    } else {
        return cacheInternal.end();
    }
}

template <>
std::vector<Cacheline>::iterator
Cache<MappingType::Direct>::chooseWhichCachelineToFillFromRAM(DecomposedAddress decomposedAddr) {
    auto cachelineToWriteInto = cacheInternal.begin() + decomposedAddr.index; // there is only one possible space
    assert(cachelineToWriteInto != cacheInternal.end());
    return cachelineToWriteInto;
}

template <>
std::vector<Cacheline>::iterator
Cache<MappingType::Fully_Associative>::chooseWhichCachelineToFillFromRAM(DecomposedAddress decomposedAddr) {
    auto firstUnusedCacheline = cacheInternal.end();
    // since there is no way for a valid cacheline to become invalid again, we can safely just fill them up one by one.
    // If this process has finished, there are sadly no more free cachelines - and there never will be again
    if (cachelineLookupTable.numCacheLinesUsed != numCacheLines) {
        firstUnusedCacheline = cacheInternal.begin() + cachelineLookupTable.numCacheLinesUsed;
        cachelineLookupTable.numCacheLinesUsed += 1;
    }

    // if there was no free cacheline :(
    if (firstUnusedCacheline == cacheInternal.end()) {
        firstUnusedCacheline = cacheInternal.begin() + replacementPolicy->pop();
        // kick out entry for tag we replaced
        cachelineLookupTable.erase(firstUnusedCacheline->tag);
    }
    assert(firstUnusedCacheline != cacheInternal.end());
    // enter us into hashtabe because we now onw this cacheline
    cachelineLookupTable[decomposedAddr.tag] = firstUnusedCacheline - cacheInternal.begin();
    return firstUnusedCacheline;
}

template <> DecomposedAddress Cache<MappingType::Direct>::decomposeAddress(std::uint32_t address) noexcept {
    assert(addressOffsetBitMask > 0 && addressIndexBitMask >= 0 && addressTagBitMask > 0);
    // modding the offset bits is presumably not necessary, but has been left in as a precaution
    return DecomposedAddress{((address >> addressOffsetBits) >> addressIndexBits) & addressTagBitMask,
                             ((address >> addressOffsetBits) & addressIndexBitMask) % numCacheLines,
                             (address & addressOffsetBitMask) % cacheLineSize};
}

template <> DecomposedAddress Cache<MappingType::Fully_Associative>::decomposeAddress(std::uint32_t address) noexcept {
    assert(addressOffsetBitMask > 0 && addressIndexBitMask == 0 && addressTagBitMask > 0);
    // modding the offset bits is presumably not necessary, but has been left in as a precaution
    return DecomposedAddress{(address >> addressOffsetBits) & addressTagBitMask, 0,
                             (address & addressOffsetBitMask) % cacheLineSize};
}

template <>
void Cache<MappingType::Direct>::registerUsage(
    __attribute__((unused)) std::vector<Cacheline>::iterator cacheline) noexcept {
    // no bookeeping needed
}

template <>
void Cache<MappingType::Fully_Associative>::registerUsage(std::vector<Cacheline>::iterator cacheline) noexcept {
    replacementPolicy->logUse(cacheline - cacheInternal.begin());
}

template <MappingType mappingType> void Cache<mappingType>::waitForRAM() noexcept {
    do {
        wait();
    } while (!writeBufferReady.read());
}

template <> void Cache<MappingType::Fully_Associative>::precomputeAddressDecompositionBits() noexcept {
    addressOffsetBits = safeCeilLog2(cacheLineSize);
    addressIndexBits = 0; // no index bits in fully associative cache
    addressTagBits = 32 - addressOffsetBits;
    assert(addressTagBits + addressIndexBits + addressOffsetBits == 32);
    addressOffsetBitMask = generateBitmaskForLowestNBits(addressOffsetBits);
    addressTagBitMask = generateBitmaskForLowestNBits(addressTagBits);
}

template <> void Cache<MappingType::Direct>::precomputeAddressDecompositionBits() noexcept {
    addressOffsetBits = safeCeilLog2(cacheLineSize);
    addressIndexBits = safeCeilLog2(numCacheLines);
    addressTagBits = 32 - addressIndexBits - addressOffsetBits;
    assert(addressTagBits + addressIndexBits + addressOffsetBits == 32);
    addressOffsetBitMask = generateBitmaskForLowestNBits(addressOffsetBits);
    addressIndexBitMask = generateBitmaskForLowestNBits(addressIndexBits);
    addressTagBitMask = generateBitmaskForLowestNBits(addressTagBits);
}

template <MappingType mappingType> void Cache<mappingType>::setUpWriteBufferConnects() noexcept {
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

template <MappingType mappingType> void Cache<mappingType>::zeroInitialiseCachelines() noexcept {
    for (auto& cacheline : cacheInternal) {
        cacheline.data = std::vector<std::uint8_t>(cacheLineSize, 0);
    }
}

template <MappingType mappingType>
Cache<mappingType>::Cache(sc_module_name name, std::uint32_t numCacheLines, std::uint32_t cacheLineSize,
                          std::uint32_t cacheLatency, std::unique_ptr<ReplacementPolicy<std::uint32_t>> policy) noexcept
    : sc_module{name}, numCacheLines{numCacheLines}, cacheLineSize{cacheLineSize}, cacheLatency{cacheLatency},
      replacementPolicy{std::move(policy)}, cacheInternal{numCacheLines},
      writeBuffer{"writeBuffer", cacheLineSize / RAM_READ_BUS_SIZE_IN_BYTE, cacheLineSize} {
    if (replacementPolicy != nullptr && mappingType == MappingType::Direct) {
        std::cerr << "Replacement Policy is set on a direct mapped cache - this has no effect.\n";
    }

    zeroInitialiseCachelines();
    precomputeAddressDecompositionBits();
    setUpWriteBufferConnects();

    SC_THREAD(handleRequest);
    sensitive << clock.pos();
}

template <MappingType mappingType>
std::vector<Cacheline>::iterator
Cache<mappingType>::writeRAMReadIntoCacheline(DecomposedAddress decomposedAddr) noexcept {
    auto cachelineToWriteInto = chooseWhichCachelineToFillFromRAM(decomposedAddr);
    writeBufferValidRequest.write(false);
    // we do not allow any inputs violating this rule in the C-part
    assert(cachelineToWriteInto->data.size() % RAM_READ_BUS_SIZE_IN_BYTE == 0);
    sc_dt::sc_bv<RAM_READ_BUS_SIZE_IN_BYTE * BITS_IN_BYTE> dataRead;
    std::uint32_t numReadEvents = (cachelineToWriteInto->data.size() / RAM_READ_BUS_SIZE_IN_BYTE);

    for (std::size_t i = 0; i < numReadEvents; ++i) {
        dataRead = writeBufferDataOut.read();
        for (std::size_t byte = 0; byte < RAM_READ_BUS_SIZE_IN_BYTE; ++byte) {
            cachelineToWriteInto->data[RAM_READ_BUS_SIZE_IN_BYTE * i + byte] =
                dataRead.range(BITS_IN_BYTE * byte + (BITS_IN_BYTE - 1), BITS_IN_BYTE * byte).to_uint();
        }
        //  if this is the last one we don't need to wait anymore
        if (i + 1 <= numReadEvents)
            wait();
    }

    cachelineToWriteInto->isValid = true;
    cachelineToWriteInto->tag = decomposedAddr.tag;

    return cachelineToWriteInto;
}

template <MappingType mappingType> void Cache<mappingType>::waitOutCacheLatency() noexcept {
    for (std::size_t i = 0; i < cacheLatency; ++i) {
        wait();
    }
}

template <MappingType mappingType>
std::vector<Cacheline>::iterator Cache<mappingType>::fetchIfNotPresent(std::uint32_t addr,
                                                                       DecomposedAddress decomposedAddr) noexcept {
    waitOutCacheLatency();
    auto cacheline = getCachelineOwnedByAddr(decomposedAddr);
    if (cacheline != cacheInternal.end()) {
        ++hitCount;
        return cacheline;
    }
    ++missCount;
    startReadFromRAM(addr);
    waitForRAM();
    return writeRAMReadIntoCacheline(decomposedAddr);
}

template <MappingType mappingType>
void Cache<mappingType>::handleSubRequest(SubRequest subRequest, std::uint32_t& readData) noexcept {
    auto addr = subRequest.addr;

    // split into tag - index - offset
    const auto decomposedAddr = decomposeAddress(addr);

    // check if in cache (waits for #cycles specified by cacheLatency) - if not read from RAM
    auto cacheline = fetchIfNotPresent(addr, decomposedAddr);

    // if a fully associative Cache and we use a stateful policy we declare the use of the cacheline here. In Direct
    // Mapped cache this is a NOP
    registerUsage(cacheline);

    if (subRequest.we) {
        doWrite(*cacheline, decomposedAddr, subRequest.data, subRequest.size);
        passWriteOnToRAM(*cacheline, decomposedAddr, addr);
    } else {
        auto tempReadData = doRead(decomposedAddr, *cacheline, subRequest.size);
        readData = applyPartialRead(subRequest, readData, tempReadData);
    }
}

template <MappingType mappingType>
std::uint32_t Cache<mappingType>::doRead(DecomposedAddress decomposedAddr, Cacheline& cacheline,
                                         std::uint32_t numBytes) noexcept {
    assert((numBytes + decomposedAddr.offset - 1) < cacheLineSize);

    std::uint32_t retVal = 0;
    for (std::size_t byteNr = 0; byteNr < numBytes; ++byteNr) {
        retVal += cacheline.data.at(decomposedAddr.offset + byteNr) << (byteNr * BITS_IN_BYTE);
    }
    return retVal;
}

template <MappingType mappingType> void Cache<mappingType>::handleRequest() noexcept {
    while (true) {
        wait();
        ready.write(false);

        if (!cpuValidRequest.read())
            continue;
        const auto request = constructRequestFromBusses();
        const auto subRequests = splitRequestIntoSubRequests(request, cacheLineSize);

        // while this is also passed into write requests, it is only relevant for read request and will not be accessed
        // if invalid for the request type
        std::uint32_t readData = 0;

        for (auto& subRequest : subRequests) {
            handleSubRequest(std::move(subRequest), readData);
        }

        if (!request.we) {
            cpuDataOutBus.write(readData);
        }
        ready.write(true);
    }
}

template <MappingType mappingType>
void Cache<mappingType>::doWrite(Cacheline& cacheline, DecomposedAddress decomposedAddr, std::uint32_t data,
                                 std::uint32_t numBytes) noexcept {
    assert((numBytes + decomposedAddr.offset - 1) < cacheLineSize);
    for (std::size_t byteNr = 0; byteNr < numBytes; ++byteNr) {
        cacheline.data[(decomposedAddr.offset + byteNr)] =
            (data >> BITS_IN_BYTE * byteNr) & generateBitmaskForLowestNBits(BITS_IN_BYTE);
    }
}

template <MappingType mappingType>
void Cache<mappingType>::passWriteOnToRAM(Cacheline& cacheline, DecomposedAddress decomposedAddr,
                                          std::uint32_t addr) noexcept {
    std::uint32_t data = 0;

    std::size_t startByte = std::min(static_cast<std::size_t>(decomposedAddr.offset), cacheline.data.size() - 4);

    data |= (cacheline.data[startByte + 0]) << 0 * BITS_IN_BYTE;
    data |= (cacheline.data[startByte + 1]) << 1 * BITS_IN_BYTE;
    data |= (cacheline.data[startByte + 2]) << 2 * BITS_IN_BYTE;
    data |= (cacheline.data[startByte + 3]) << 3 * BITS_IN_BYTE;

    writeBufferAddr.write((startByte == static_cast<std::size_t>(decomposedAddr.offset)
                               ? (addr)
                               : ((addr / cacheLineSize) * cacheLineSize + cacheline.data.size() - 4)));
    writeBufferDataIn.write(data);
    writeBufferWE.write(true);
    writeBufferValidRequest.write(true);

    wait();
    writeBufferValidRequest.write(false); // ensure we only set valid for exactly one cycle
    while (!writeBufferReady) {
        wait();
    }

#ifdef STRICT_INSTRUCTION_ORDER
    for (std::uint32_t waited = 0; waited < memoryLatency; ++waited) {
        wait();
    }
#endif
}

template <MappingType mappingType> void Cache<mappingType>::startReadFromRAM(std::uint32_t addr) noexcept {
    std::uint32_t alignedAddr = (addr / cacheLineSize) * cacheLineSize;
    writeBufferAddr.write(alignedAddr);
    writeBufferWE.write(false);
    writeBufferValidRequest.write(true);
}

template <MappingType mappingType> Request Cache<mappingType>::constructRequestFromBusses() const noexcept {
    return Request{cpuAddrBus.read(), cpuDataInBus.read(), cpuWeBus.read()};
}

static constexpr std::size_t calcGateCountForInternalTable(std::uint32_t numCachelines, std::uint32_t cachelineSize,
                                                           std::uint32_t tagBits) noexcept {
    // each bit register takes 4 gates
    // we have 8 bits in a byte and numCachelines * cachelinesize bytes + tag bits
    return mulSatUnsigned(static_cast<size_t>(4), static_cast<size_t>(numCachelines),
                          addSatUnsigned(mulSatUnsigned(static_cast<size_t>(8), static_cast<size_t>(cachelineSize)),
                                         static_cast<size_t>(tagBits)));
}

static constexpr std::size_t
calcGateCountForCachelineSelection(std::uint32_t numCachelines, std::uint32_t cacheLineSize, std::uint32_t tagBits,
                                   MappingType type, const ReplacementPolicy<std::uint32_t>& policy) noexcept {
    // a selector built like shown here. https://learn.sparkfun.com/tutorials/how-does-an-fpga-work/multiplexers
    // making it numCachelines*cacheLineSize*8 AND Gates and cacheLineSize*8 Or GAtes with numCachelines Inputs (:=
    // 1 primitive gate)
    std::size_t selector =
        addSatUnsigned(mulSatUnsigned(static_cast<size_t>(numCachelines), static_cast<size_t>(cacheLineSize),
                                      static_cast<size_t>(BITS_IN_BYTE)),
                       mulSatUnsigned(static_cast<size_t>(BITS_IN_BYTE), static_cast<size_t>(cacheLineSize)));
    // decomposing addr := 1
    std::size_t decomposingAddr = 1;

    if (type == MappingType::Fully_Associative) {
        // we use a hashtable to find out which cacheline a given tag belongs to. As shown in this paper:
        // https://ar5iv.labs.arxiv.org/html/2108.03390v2 such a hashtable can be implemented on a Intel Stratix 10
        // GX1800 FPGA. As shown here:
        // https://www.intel.com/content/www/us/en/products/sku/210291/intel-stratix-10-gx-2800-fpga/specifications.html
        // such an FPGA has 2753000 "logic elements", which we eqaute to primitive gates here.
        std::size_t FPGA = 2753000u;
        // a 32 bit register to store the amount of filled cachelines
        std::size_t validCachelineCntr = BITS_IN_BYTE * 32u;
        return addSatUnsigned(FPGA, validCachelineCntr, decomposingAddr, selector);
    } else {
        return addSatUnsigned(decomposingAddr, selector);
    }
}

template <MappingType mappingType> std::size_t Cache<mappingType>::calculateGateCount() const {
    return addSatUnsigned(calcGateCountForCachelineSelection(numCacheLines, cacheLineSize, addressTagBits, mappingType,
                                                             *replacementPolicy),
                          calcGateCountForInternalTable(numCacheLines, cacheLineSize, addressTagBits));
}
#ifdef STRICT_INSTRUCTION_ORDER
template <MappingType mappingType> void Cache<mappingType>::setMemoryLatency(std::uint32_t memoryLatency) {}
#endif

template <MappingType mappingType> void Cache<mappingType>::traceInternalSignals(sc_trace_file* const traceFile) const {
    sc_trace(traceFile, writeBufferReady, "WriteBuffer_Cache_Ready");
    sc_trace(traceFile, writeBufferDataOut, "WriteBuffer_Cache_Data_Out");
    sc_trace(traceFile, writeBufferAddr, "Cache_WriteBuffer_Addr");
    sc_trace(traceFile, writeBufferDataIn, "Cache_WriteBuffer_Data_in");
    sc_trace(traceFile, writeBufferWE, "Cache_WriteBuffer_WE");
    sc_trace(traceFile, writeBufferValidRequest, "Cache_WriteBuffer_Valid_Request");
}

template struct Cache<MappingType::Direct>;
template struct Cache<MappingType::Fully_Associative>;