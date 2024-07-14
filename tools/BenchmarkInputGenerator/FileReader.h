#pragma once

#include <cstdint>
#include <vector>
#include <cstdio>
#include <cstdlib>


std::vector<std::int64_t> readInValues(char* filename, int valueCount) {
    FILE* file = fopen(filename, "r");
    std::vector<std::int64_t> nums;
    std::int64_t value;

    for (int i = 0; i < valueCount; ++i) {
        fscanf(file, "%lli", &value);
        nums.push_back(value);
    }

    fclose(file);
    return nums;
}
