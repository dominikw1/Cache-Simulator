#include "Utils.h"

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <random>
#include <unordered_set>

std::vector<std::uint64_t> generateRandomVector(std::uint64_t len) {
    std::vector<std::uint64_t> vec(len, 0);
    std::srand(std::time(nullptr) + rand());
    std::generate(vec.begin(), vec.end(), []() { return std::rand(); });
    return vec;
}

std::vector<std::uint64_t> generateRandomVector(std::uint64_t len, std::uint64_t max) {
    auto vec = generateRandomVector(len);
    std::transform(vec.cbegin(), vec.cend(), vec.begin(), [max](const std::uint64_t v) { return v % max; });
    return vec;
}

std::vector<std::uint64_t> makeVectorUniqueNoOrderPreserve(std::vector<std::uint64_t> input) {
    std::unordered_set<std::uint64_t> set(input.cbegin(), input.cend());
    input.assign(set.cbegin(), set.cend());
    std::shuffle(input.begin(), input.end(), std::default_random_engine{});
    return input;
}
