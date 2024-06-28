#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc == 1) {
        return -1;
    }
    const long long numToGenerate = std::atoll(argv[1]);
    std::srand(unsigned(std::time(nullptr)));
    std::vector<int> v(numToGenerate);
    std::generate(v.begin(), v.end(), std::rand);
    std::ofstream outFile("randomNumbers_" + std::to_string(numToGenerate) + ".txt");
    for (const auto& e : v)
        outFile << e << " ";
}