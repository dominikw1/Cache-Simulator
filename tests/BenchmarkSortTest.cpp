#include "../tools/BenchmarkInputGenerator/Sort.h"
#include "Utils.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

TEST(BenchmarkSortTests, RadixSortSortsCorrently) {
    for (std::size_t i = 0; i < 1000; ++i) {
        auto vecWrongFormat = generateRandomVector(rand() % 10000);
        std::vector<std::int64_t> vec(vecWrongFormat.size());
        transform(vecWrongFormat.begin(), vecWrongFormat.end(), vec.begin(),
                  [](std::uint64_t in) { return static_cast<std::int64_t>(in); });
        auto controlVec = vec;
        radixSort(&vec[0], vec.size());
        std::sort(controlVec.begin(), controlVec.end());
        ASSERT_EQ(controlVec, vec);
    }
}

TEST(BenchmarkSortTests, MergeSortSortsCorrently) {
    for (std::size_t i = 0; i < 1000; ++i) {
        auto vecWrongFormat = generateRandomVector(rand() % 10000);
        std::vector<std::int64_t> vec(vecWrongFormat.size());
        transform(vecWrongFormat.begin(), vecWrongFormat.end(), vec.begin(),
                  [](std::uint64_t in) { return static_cast<std::int64_t>(in); });
        auto controlVec = vec;
        mergeSort(&vec[0], 0, vec.size() - 1);
        std::sort(controlVec.begin(), controlVec.end());
        ASSERT_EQ(controlVec, vec);
    }
}