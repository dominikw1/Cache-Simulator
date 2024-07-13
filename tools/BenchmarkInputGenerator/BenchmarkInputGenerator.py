import os
def generateRandomNumFile(num: int) -> None:
    os.system(f"./../RandomNumberGenerator/a.out {num}")

def renameRandomNumFile(num: int, newName: str) -> None:
    os.system(f"cp randomNumbers_{num}.txt {newName}")

def createBenchmarkInput(num: int) -> None:
    generateRandomNumFile(num)
    renameRandomNumFile(num, "input.txt")
    os.system("./mergeSort.out > /dev/null")
    os.system(f"cp memory_analysis.csv Benchmarks/merge_sort_{num}.csv")
    os.system("./radixSort.out > /dev/null")
    os.system(f"cp memory_analysis.csv Benchmarks/radix_sort_{num}.csv")
    

os.system("mkdir -p Benchmarks")
createBenchmarkInput(100)
createBenchmarkInput(1000)
createBenchmarkInput(10000)