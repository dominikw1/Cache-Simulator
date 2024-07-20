from pathlib import Path
import subprocess
from dataclasses import dataclass
import re
from typing import List

pathToCacheExe=  "../cache"


@dataclass
class RawResult:
    cyclesNeeded: int
    hits: int
    misses: int
    gates: int
    
@dataclass
class BenchmarkResult:
    inputSize: int
    alg: str
    policy: str
    direct_mapped: bool
    cacheLineSize: int
    cacheLineNum: int
    cacheLatency: int
    memLatency: int
    result: RawResult


def printAsCSV(headers: List[str], values, name) -> None:
    output: str = ",".join(headers)+ "\n"
    for i in range(len(values[0])):
        output += ",".join([str(v[i]) for v in values])+str('\n')
    with open(name, "w") as file:
          file.write(output)


def extractRawRes(res: str) -> RawResult:
    lines = res.split('\n')
    cycles  =int([line.strip() for line in lines if line.strip().startswith("Cycles:\t")][0].removeprefix("Cycles:\t"))
    misses  =int([line.strip() for line in lines if line.strip().startswith("Misses:\t")][0].removeprefix("Misses:\t"))
    
    hits  =int([line.strip() for line in lines if line.strip().startswith("Hits:\t")][0].removeprefix("Hits:\t"))
    gates = int([line.strip() for line in lines if line.strip().startswith("Primitive gate count:\t")][0].removeprefix("Primitive gate count:\t"))
    return RawResult(cycles,hits, misses, gates)
    
def runBenchmark(input: Path, cacheSize: int, cacheLineNum: int, memLatency: int, cacheLatency: int, policy: str = "lru", direct_mapped: bool= False) -> str:
    cmd = [pathToCacheExe, input, "--lcycles", "-c", str(4294967295), "--cache-latency" ,str(cacheLatency), "--memory-latency",str(memLatency) ,"--cachelines",str(cacheLineNum), "--cacheline-size", str(cacheSize) , f"--{policy}"]
    if direct_mapped:
        cmd += ["--directmapped"]
    #print(cmd)
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    res = extractRawRes(proc.communicate()[0].decode("utf-8"))
    
    return res

def runBenchmarkForAlg(alg: str, *, cacheLineSize: int, cacheLineNum: int, memLatency: int, cacheLatency: int)-> str:
    bs = []
    for len in {10, 100, 1000}:
        r = runBenchmark(f"BenchmarkInputGenerator/Benchmarks/{alg}_sort_{len}.csv", cacheLineSize, cacheLineNum, memLatency, cacheLatency,direct_mapped=True) 
        bs.append(BenchmarkResult(len, alg, cacheLatency=cacheLatency, memLatency=memLatency, result=r, cacheLineNum=cacheLineNum, cacheLineSize=cacheLineSize, direct_mapped=True, policy="lru"))             
    return bs

def runBenchmarkForCacheSize(*,  cacheLineNum: int, memLatency: int, cacheLatency: int)-> str:
    bs = []
    for cacheLineSize  in {16,32,64, 128, 256, 512}:
        r = runBenchmark(f"BenchmarkInputGenerator/Benchmarks/merge_sort_100.csv", cacheLineNum=cacheLineNum, memLatency=memLatency, cacheLatency=cacheLatency, cacheSize=cacheLineSize) 
        bs.append(BenchmarkResult(100, "merge", cacheLatency=cacheLatency, memLatency=memLatency, result=r, cacheLineNum=cacheLineNum, cacheLineSize=cacheLineSize, direct_mapped=False, policy="lru"))             
    return bs

def runBenchmarkForPolicy(*, cacheLineNum: int, memLatency: int, cacheLatency: int, cacheLineSize: int):
    bs = []
    for policyI in {"lru", "fifo", "random"}:
        r = runBenchmark(f"BenchmarkInputGenerator/Benchmarks/merge_sort_100.csv", cacheLineNum=cacheLineNum, memLatency=memLatency, cacheLatency=cacheLatency, cacheSize=cacheLineSize, policy=policyI, direct_mapped=False) 
        bs.append(BenchmarkResult(100, "merge", policy=policyI, direct_mapped=False, cacheLatency=cacheLatency, memLatency=memLatency, result=r, cacheLineNum=cacheLineNum, cacheLineSize=cacheLineSize))             
    return bs

def runBenchmarkForMappingType(*, cacheLineNum: int, memLatency: int, cacheLatency: int, cacheLineSize: int):
    bs = []
    for directMapped in {True, False}:
        r = runBenchmark(f"BenchmarkInputGenerator/Benchmarks/merge_sort_100.csv", cacheLineNum=cacheLineNum, memLatency=memLatency, cacheLatency=cacheLatency, cacheSize=cacheLineSize, policy="lru", direct_mapped=directMapped) 
        bs.append(BenchmarkResult(100, "merge", policy="lru", direct_mapped=directMapped, cacheLatency=cacheLatency, memLatency=memLatency, result=r, cacheLineNum=cacheLineNum, cacheLineSize=cacheLineSize))             
    return bs

print("===== Merge Sort vs. Radix Sort =====")
mergeBenches: list[BenchmarkResult] = runBenchmarkForAlg("merge", cacheLineSize=16, cacheLineNum=8, memLatency=100, cacheLatency=20)
radixBenches: list[BenchmarkResult] = runBenchmarkForAlg("radix", cacheLineSize=16, cacheLineNum=8, memLatency=100, cacheLatency=20)

benches = mergeBenches + radixBenches

for b in benches:
    print(f"Alg: {b.alg}, Size: {b.inputSize}:")
    print(f"Cycles: {b.result.cyclesNeeded}, Total memory accesses: {b.result.misses + b.result.hits}")
    print(f"Hit-%: {100*b.result.hits / (b.result.hits+b.result.misses)}%, Miss-%: {100*b.result.misses / (b.result.misses+b.result.hits)}%")
    print(f"Cycles/Memory Access: {b.result.cyclesNeeded  / ((b.result.hits+b.result.misses))}")
    
printAsCSV(["Algorithm", "Size", "Hit-%", "Cycles/M.A."], [[b.alg for b in benches] ,[b.inputSize for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "algBenchmarks.csv")

print("===== Increasing Cache-Line size =====")
benches: list[BenchmarkResult] = runBenchmarkForCacheSize(cacheLineNum=8, memLatency=100, cacheLatency=5)
for b in benches:
    print(f"Cache-Line size: {b.cacheLineSize}")
    print(f"Cycles: {b.result.cyclesNeeded}, Total memory accesses: {b.result.misses + b.result.hits}")
    print(f"Hit-%: {100*b.result.hits / (b.result.hits+b.result.misses)}%, Miss-%: {100*b.result.misses / (b.result.misses+b.result.hits)}%")
    print(f"Cycles/Memory Access: {b.result.cyclesNeeded  / ((b.result.hits+b.result.misses))}")
    print(f"Gates: {b.result.gates}\n")

printAsCSV(["Cacheline-Size", "Hit-%", "Cycles/M.A."], [[b.cacheLineSize for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "cacheLineSizeBenchmarksFastMem.csv")


benches: list[BenchmarkResult] = runBenchmarkForCacheSize(cacheLineNum=8, memLatency=500, cacheLatency=5)
for b in benches:
    print(f"Cache-Line size: {b.cacheLineSize}")
    print(f"Cycles: {b.result.cyclesNeeded}, Total memory accesses: {b.result.misses + b.result.hits}")
    print(f"Hit-%: {100*b.result.hits / (b.result.hits+b.result.misses)}%, Miss-%: {100*b.result.misses / (b.result.misses+b.result.hits)}%")
    print(f"Cycles/Memory Access: {b.result.cyclesNeeded  / ((b.result.hits+b.result.misses))}")
    print(f"Gates: {b.result.gates}\n")

printAsCSV(["Cacheline-Size", "Hit-%", "Cycles/M.A."], [[b.cacheLineSize for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "cacheLineSizeBenchmarksSlowMem.csv")



print("===== Replacement Policy =====")
benches: list[BenchmarkResult] = runBenchmarkForPolicy(cacheLineNum=8, cacheLineSize=16, memLatency=100, cacheLatency=5)
for b in benches:
    print(f"Policy: {b.policy}")
    print(f"Cycles: {b.result.cyclesNeeded}, Total memory accesses: {b.result.misses + b.result.hits}")
    print(f"Hit-%: {100*b.result.hits / (b.result.hits+b.result.misses)}%, Miss-%: {100*b.result.misses / (b.result.misses+b.result.hits)}%")
    print(f"Cycles/Memory Access: {b.result.cyclesNeeded  / ((b.result.hits+b.result.misses))}")
    print(f"Gates: {b.result.gates}\n")

printAsCSV(["Replacement-Policy", "Hit-%", "Cycles/M.A."], [[b.policy for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "policyBenchmarks.csv")

    
    
print("===== Mapping Type =====")
benches: list[BenchmarkResult] = runBenchmarkForMappingType(cacheLineNum=8, cacheLineSize=16, memLatency=100, cacheLatency=5)
for b in benches:
    if(b.direct_mapped):
        print("Direct-Mapped\n")
    else:
        print("Fully Associative\n")
    print(f"Cycles: {b.result.cyclesNeeded}, Total memory accesses: {b.result.misses + b.result.hits}")
    print(f"Hit-%: {100*b.result.hits / (b.result.hits+b.result.misses)}%, Miss-%: {100*b.result.misses / (b.result.misses+b.result.hits)}%")
    print(f"Cycles/Memory Access: {b.result.cyclesNeeded  / ((b.result.hits+b.result.misses))}")
    print(f"Gates: {b.result.gates}\n")
    
printAsCSV(["Mapping-Type", "Hit-%", "Cycles/M.A."], [["Direct" if b else "Fully-Associative"  for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "mappingBenchmarks.csv")

    
