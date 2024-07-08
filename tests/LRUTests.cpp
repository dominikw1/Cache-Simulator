#include <gtest/gtest.h>

#include "../src/LRUPolicy.h"
#include "Utils.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <unordered_set>
#include <vector>

TEST(LRUPolicyTests, LRUPolicyAddingSingleValueReturnsSameValue) {
    LRUPolicy<std::uint64_t> LRUPolicy{100};
    LRUPolicy.logUse(10);
    ASSERT_EQ(10, LRUPolicy.popLRUPolicy());
}

TEST(LRUPolicyTests, LRUPolicyAddingMultipleUniqueValuesReturnsInCorrectOrder) {
    LRUPolicy<std::uint64_t> LRUPolicy{10000};
    auto randomVec = makeVectorUniqueNoOrderPreserve(generateRandomVector(10000));
    std::for_each(randomVec.cbegin(), randomVec.cend(), [&LRUPolicy](std::uint64_t val) { LRUPolicy.logUse(val); });
    ASSERT_EQ(randomVec.size(), LRUPolicy.getSize());
    for (std::size_t i = 0; i < randomVec.size(); ++i) {
        ASSERT_EQ(randomVec[i], LRUPolicy.popLRUPolicy());
    }
}

TEST(LRUPolicyTests, LRUPolicyAddingFewNonUniqueValuesReturnsInCorrectOrder) {
    LRUPolicy<bool> LRUPolicy{2};
    auto inputList = std::vector<bool>{true, true, false, true};
    std::for_each(inputList.cbegin(), inputList.cend(), [&LRUPolicy](bool val) { LRUPolicy.logUse(val); });
    ASSERT_EQ(false, LRUPolicy.popLRUPolicy());
    ASSERT_EQ(true, LRUPolicy.popLRUPolicy());
}

static bool existsDuplicate(const std::vector<std::uint64_t>& vec) {
    std::unordered_set<int> s(vec.cbegin(), vec.cend());
    return s.size() != vec.size();
}

TEST(LRUPolicyTests, LRUPolicyAddingManyNonUniqueValuesReturnsInCorrectOrder) {
    LRUPolicy<std::uint64_t> LRUPolicy{100000};
    auto inputList = generateRandomVector(100000, 1000);
    ASSERT_TRUE(existsDuplicate(inputList)); // pigeon-hole principle
    std::for_each(inputList.cbegin(), inputList.cend(), [&LRUPolicy](std::uint64_t val) { LRUPolicy.logUse(val); });

    std::vector<std::uint64_t> popped{};
    while (LRUPolicy.getSize() > 0)
        popped.push_back(LRUPolicy.popLRUPolicy());

    // LRUPolicy does not keep state -> we can just iterate from the back
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
