#include "../MemoryAnalyser/include/MemoryAnalyserSupport.h"
#include "Sort.h"
#include "FileReader.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        return -1;
    }

    char* filename = argv[1];
    int valueCount = atoi(argv[2]);

    std::vector<std::int64_t> values = readInValues(filename, valueCount);

    radixSort(&(values[0]), valueCount - 1);
    for (auto& v: values) {
        printf("%lld ", v); // to ensure compiler cant optimise sort away
    }
    return 0;
}
