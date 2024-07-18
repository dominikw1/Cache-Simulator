import os

def generateRandomNumFile(num: int) -> None:
    os.system(f"./../RandomNumberGenerator/a.out {num}")

def normaliseBenchmarkInput(file: str) -> None:
    with open(file, 'r') as openFile:
        lines = openFile.read().split('\n')
        
        wes = [line.split(',')[0] for line in lines if line.strip()]
        addresses = [int(line.split(',')[1],0) for line in lines if line.strip() ]
        data = [int(line.split(',')[2],0) if (line.strip() and line.split(',')[2] != '') else None for line in lines ]
        outStr = ""
        minAddr = min(addresses)
        for l in zip(wes, addresses, data):
            if l[2] != None:
                outStr += f"{l[0]},{hex((l[1]-minAddr)%0xfffffff)},{l[2]%0xffffffff}\n"
            else:
                 outStr += f"{l[0]},{hex((l[1]-minAddr)%0xfffffff)},\n"
            
    with open(file, "w") as openFile:
        openFile.write(outStr)
                    

def createBenchmarkInput(num: int) -> None:
    generateRandomNumFile(num)
    os.system(f"./mergeSort.out randomNumbers_{num}.txt {num} > /dev/null")
    os.system(f"cp memory_analysis.csv Benchmarks/merge_sort_{num}.csv")
    normaliseBenchmarkInput(f"Benchmarks/merge_sort_{num}.csv")
    os.system(f"rm memory_analysis.csv")
    os.system(f"./radixSort.out randomNumbers_{num}.txt {num} > /dev/null")
    os.system(f"cp memory_analysis.csv Benchmarks/radix_sort_{num}.csv")
    normaliseBenchmarkInput(f"Benchmarks/radix_sort_{num}.csv")
    os.system(f"rm memory_analysis.csv")



os.system("mkdir -p Benchmarks")
createBenchmarkInput(10)
createBenchmarkInput(100)
createBenchmarkInput(1000)
