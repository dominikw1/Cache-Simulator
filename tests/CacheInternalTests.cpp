#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <limits>
#include <cstddef>

#include "../src/CacheInternal.h"
#include "../src/InternalRequests.h"
#include "Utils.h"

TEST(CacheInternalTests, CPUQueryAtStartReturns0) {
    CacheInternal internals{1, 10};
    ASSERT_EQ(0, internals.handleCPURead(InternalRequestCPURead{0, 0}));
    ASSERT_EQ(0, internals.handleCPURead(InternalRequestCPURead{0, 6}));
    ASSERT_EQ(0, internals.handleCPURead(InternalRequestCPURead{0, 3}));
    ASSERT_EQ(0, internals.handleCPURead(InternalRequestCPURead{0, 4}));
}

TEST(CacheInternalTests, CPUQueryReturnsPreviouslySet) {
    CacheInternal internals{1, 100};
    auto addresses = generateRandomVector(10000, 92);
    auto values = generateRandomVector(10000, std::numeric_limits<uint32_t>::max());
    const auto test = [&internals](std::uint64_t addr, std::uint64_t val) {
        std::cout<<"Writing "<<val<<" to "<<addr<<std::endl;
        internals.handleCPUWrite(
            InternalRequestCPUWrite{0, static_cast<std::uint32_t>(addr), static_cast<std::uint32_t>(val)});
        ASSERT_EQ(static_cast<std::uint32_t>(val),
                  internals.handleCPURead(InternalRequestCPURead{0, static_cast<std::uint32_t>(addr)}));
    };
    //std::transform(addresses.begin(), addresses.end(), values.begin(), values.end(), fun);
    for(std::size_t i = 0; i<addresses.size(); ++i) {
        test(addresses[i], values[i]);
    }
}