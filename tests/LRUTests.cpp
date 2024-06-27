#include <gtest/gtest.h>

#include "../src/LRU.h"
#include "Utils.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <unordered_set>
#include <vector>

TEST(LRUTests, LRUAddingSingleValueReturnsSameValue) {
    LRU<std::uint64_t> lru{100};
    lru.logUse(10);
    ASSERT_EQ(10, lru.popLRU());
}

TEST(LRUTests, LRUAddingMultipleUniqueValuesReturnsInCorrectOrder) {
    LRU<std::uint64_t> lru{10000};
    auto randomVec =
        makeVectorUniqueNoOrderPreserve(generateRandomVector(10000));
    std::for_each(randomVec.cbegin(), randomVec.cend(),
                  [&lru](std::uint64_t val) { lru.logUse(val); });
    ASSERT_EQ(randomVec.size(), lru.getSize());
    for (std::size_t i = 0; i < randomVec.size(); ++i) {
        ASSERT_EQ(randomVec[i], lru.popLRU());
    }
}

TEST(LRUTests, LRUAddingFewNonUniqueValuesReturnsInCorrectOrder) {
    LRU<bool> lru{2};
    auto inputList = std::vector<bool>{true, true, false, true};
    std::for_each(inputList.cbegin(), inputList.cend(),
                  [&lru](bool val) { lru.logUse(val); });
    ASSERT_EQ(false, lru.popLRU());
    ASSERT_EQ(true, lru.popLRU());
}

static bool existsDuplicate(const std::vector<std::uint64_t>& vec) {
    std::unordered_set<int> s(vec.cbegin(), vec.cend());
    return s.size() != vec.size();
}

TEST(LRUTests, LRUAddingManyNonUniqueValuesReturnsInCorrectOrder) {
    LRU<std::uint64_t> lru{100000};
    auto inputList = generateRandomVector(100000, 1000);
    ASSERT_TRUE(existsDuplicate(inputList)); // pigeon-hole principle
    std::for_each(inputList.cbegin(), inputList.cend(),
                  [&lru](std::uint64_t val) { lru.logUse(val); });

    std::vector<std::uint64_t> popped{};
    while (lru.getSize() > 0)
        popped.push_back(lru.popLRU());

    // lru does not keep state -> we can just iterate from the back
    std::reverse(popped.begin(), popped.end());
    auto inputListIt = inputList.rbegin();
    std::unordered_set<std::uint64_t> alreadySeen{};

    for (const auto& el : popped) {
        while (alreadySeen.count(*inputListIt) > 0) {
            inputListIt++;
            continue;
        }
        ASSERT_EQ(*inputListIt, el);
        alreadySeen.insert(*inputListIt);
        inputListIt++;
    }
}
