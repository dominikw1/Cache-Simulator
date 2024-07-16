import os
from pathlib import Path
def generateRandomNumFile(num: int) -> None:
    os.system(f"./../RandomNumberGenerator/a.out {num}")

def createBenchmarkInput(num: int) -> None:
    generateRandomNumFile(num)
    os.system(f"./mergeSort.out randomNumbers_{num}.txt {num} > /dev/null")
    os.system(f"cp memory_analysis.csv Benchmarks/merge_sort_{num}.csv")
    os.system(f"rm memory_analysis.csv")
    os.system(f"./radixSort.out randomNumbers_{num}.txt {num} > /dev/null")
    os.system(f"cp memory_analysis.csv Benchmarks/radix_sort_{num}.csv")
    os.system(f"rm memory_analysis.csv")

pathToCacheExe=  "../../cache"

def runBenchmark(input: Path, cacheSize: int, cacheLineNum: int) -> None:
    os.system(f".{pathToCacheExe} -c 0xFFFFFFFF --cachelines {cacheLineNum} --cacheline-size {cacheSize}")

os.system("mkdir -p Benchmarks")
createBenchmarkInput(10)
createBenchmarkInput(100)
createBenchmarkInput(1000)
createBenchmarkInput(10000)

runBenchmark("Benchmarks/radix_sort_10.csv", 64, 10)




