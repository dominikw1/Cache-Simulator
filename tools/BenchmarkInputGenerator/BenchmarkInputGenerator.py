import os
def generateRandomNumFile(num: int) -> None:
    os.system(f"./../RandomNumberGenerator/a.out {num}")

def createBenchmarkInput(num: int) -> None:
    generateRandomNumFile(num)
    os.system(f"./mergeSort.out randomNumbers_{num}.txt {num} > /dev/null")
    os.system(f"cp memory_analysis.csv Benchmarks/merge_sort_{num}.csv")
    os.system(f"./radixSort.out randomNumbers_{num}.txt {num} > /dev/null")
    os.system(f"cp memory_analysis.csv Benchmarks/radix_sort_{num}.csv")
    

os.system("mkdir -p Benchmarks")
createBenchmarkInput(100)
createBenchmarkInput(1000)
createBenchmarkInput(10000)