#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

std::vector<std::int64_t> readInValues(char* filename, int valueCount) {
    FILE* file = fopen(filename, "r");
    std::vector<std::int64_t> nums(valueCount);
    std::int64_t value;

    for (int i = 0; i < valueCount; ++i) {
        fscanf(file, "%lli", &value);
        nums[i] = value;
    }

    fclose(file);
    return nums;
}
