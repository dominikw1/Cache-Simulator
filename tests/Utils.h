#pragma once

#include <cstdint>
#include <vector>

std::vector<std::uint64_t> generateRandomVector(std::uint64_t len);
std::vector<std::uint64_t> generateRandomVector(std::uint64_t len, std::uint64_t max);
std::vector<std::uint64_t> makeVectorUniqueNoOrderPreserve(std::vector<std::uint64_t> input);
