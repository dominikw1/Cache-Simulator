#include <gtest/gtest.h>

#include "../src/Policy/FIFOPolicy.h"
#include "Utils.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <set>
#include <unordered_set>
#include <vector>

TEST(FIFOPolicyTests, FIFOPolicyAddingSingleValueReturnsSameValue) {
    FIFOPolicy<std::uint64_t> fifoPolicy{100};
    fifoPolicy.logUse(10);
    ASSERT_EQ(10, fifoPolicy.pop());
}

TEST(FIFOPolicyTests, FIFOPolicyAddingMultipleUniqueValuesReturnsInCorrectOrder) {
    FIFOPolicy<std::uint64_t> fifoPolicy{10000};
    auto randomVec = makeVectorUniqueNoOrderPreserve(generateRandomVector(10000));
    std::for_each(randomVec.cbegin(), randomVec.cend(), [&fifoPolicy](std::uint64_t val) { fifoPolicy.logUse(val); });
    ASSERT_EQ(randomVec.size(), fifoPolicy.getSize());
    for (std::size_t i = 0; i < randomVec.size(); ++i) {
        ASSERT_EQ(randomVec[i], fifoPolicy.pop());
    }
}

TEST(FIFOPolicyTests, FIFOolicyAddingFewNonUniqueValuesReturnsInCorrectOrder) {
    FIFOPolicy<bool> fifoPolicy{2};
    auto inputList = std::vector<bool>{true, true, false, true};
    std::for_each(inputList.cbegin(), inputList.cend(), [&fifoPolicy](bool val) { fifoPolicy.logUse(val); });
    ASSERT_EQ(true, fifoPolicy.pop());
    ASSERT_EQ(false, fifoPolicy.pop());
}

TEST(FIFOPolicyTests, FIFOCorrectOrderOfUniqueAfterLotsOfRandomInputs) {
    FIFOPolicy<std::uint64_t> fifoPolicy{100000};
    auto inputList = generateRandomVector(100000, 1000);
    std::for_each(inputList.cbegin(), inputList.cend(), [&fifoPolicy](std::uint64_t val) { fifoPolicy.logUse(val); });
    std::size_t trueSize = fifoPolicy.getSize();
    for(std::size_t i = 0; i< trueSize; ++i) {
        fifoPolicy.pop();
    }
    ASSERT_EQ(fifoPolicy.getSize(), 0);

    auto toTestInputList = std::vector<std::uint64_t>(100000);
    std::iota(toTestInputList.begin(), toTestInputList.end(), 0);
    std::for_each(toTestInputList.cbegin(), toTestInputList.cend(),
                  [&fifoPolicy](std::uint64_t val) { fifoPolicy.logUse(val); });

    std::vector<std::uint64_t> popped{};
    while (fifoPolicy.getSize() > 0)
        popped.push_back(fifoPolicy.pop());

    ASSERT_EQ(popped.size(), toTestInputList.size());
    for (int i = 0; i < popped.size(); ++i) {
        ASSERT_EQ(popped[i], toTestInputList[i]);
    }
}

TEST(RingQueueTest, AddBoutsManyTimesWorksCorrectly) {
    RingQueue<std::uint64_t> q{153};
    for (int i = 0; i < 1000; ++i) {
        auto vecToAdd = generateRandomVector(100);
        std::for_each(vecToAdd.begin(), vecToAdd.end(), [&q](std::uint64_t el) { q.push(el); });
        for (auto el : vecToAdd) {
            ASSERT_EQ(el, q.pop());
        }
    }
}

TEST(RingQueueTest, AddSmallAmountWorks) {
    RingQueue<std::uint64_t> q{4};
    q.push(3);
    q.push(5);
    ASSERT_EQ(3, q.pop());
    ASSERT_EQ(5, q.pop());
}

TEST(RingQueueTest, AddSingleValWorks) {
    RingQueue<std::uint64_t> q{4};
    ASSERT_EQ(q.getCapacity(), 4);
    q.push(4);
    ASSERT_EQ(4, q.pop());
}