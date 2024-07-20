#pragma once

#include "../src/Request.h"

#include <cstdint>
#include <vector>

std::vector<std::uint64_t> generateRandomVector(std::uint64_t len);
std::vector<std::uint64_t> generateRandomVector(std::uint64_t len, std::uint64_t max);
std::vector<std::uint64_t> makeVectorUniqueNoOrderPreserve(std::vector<std::uint64_t> input);
Request* generateRandomRequests(std::uint64_t len);
Request* generateRandomRequests(std::uint64_t len, std::uint64_t addressMax, std::uint64_t dataMax, std::uint64_t weMax);
