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
    misses  =int([line.strip() for line in lines if line.strip().startswith("Misses:\t")][0].removeprefix("Misses:\t\x1b[31m").removesuffix("\t\t\x1b[0m"))
    
    hits  =int([line.strip() for line in lines if line.strip().startswith("Hits:\t")][0].removeprefix("Hits:\t\x1b[32m").removesuffix("\t\t\x1b[0m"))
    gates = int([line.strip() for line in lines if line.strip().startswith("Primitive gate count:\t")][0].removeprefix("Primitive gate count:\t").removesuffix("\x1b[0m"))
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

def runBenchmarkForPolicyAndCacheLineNum(*, memLatency: int, cacheLatency: int, cacheLineSize: int):
    bs = []
    for cachelineNum in {10, 100, 1000}:
        bs+=(runBenchmarkForPolicy(memLatency=memLatency, cacheLineNum=cachelineNum, cacheLatency=cacheLatency, cacheLineSize=cacheLineSize))
    return bs

def runBenchmarkForMappingType(*, cacheLineNum: int, memLatency: int, cacheLatency: int, cacheLineSize: int, alg: str = "merge"):
    bs = []
    for directMapped in {True, False}:
        r = runBenchmark(f"BenchmarkInputGenerator/Benchmarks/{alg}_sort_100.csv", cacheLineNum=cacheLineNum, memLatency=memLatency, cacheLatency=cacheLatency, cacheSize=cacheLineSize, policy="lru", direct_mapped=directMapped) 
        bs.append(BenchmarkResult(100, alg, policy="lru", direct_mapped=directMapped, cacheLatency=cacheLatency, memLatency=memLatency, result=r, cacheLineNum=cacheLineNum, cacheLineSize=cacheLineSize))             
    return bs

def runBenchmarkForMappingTypeVaryingCacheLineNum(*, memLatency: int, cacheLatency: int, cacheLineSize: int):
    bs = []
    for cacheLineNum in {10, 100, 1000, 10000}:
        bs += runBenchmarkForMappingType(cacheLineNum=cacheLineNum, memLatency=memLatency, cacheLatency=cacheLatency, cacheLineSize=cacheLineSize)
    return bs

def runBenchmarkForMappingTypeVaryingCacheLineNumAndCacheLineSize(*, memLatency: int, cacheLatency: int):
    bs = []
    for cacheLineSize in {16, 32, 64, 128}:
        for cacheLineNum in {10, 100, 1000, 10000}:
            bs += runBenchmarkForMappingType(cacheLineNum=cacheLineNum, memLatency=memLatency, cacheLatency=cacheLatency, cacheLineSize=cacheLineSize)
    return bs


def runBenchmarkForMappingTypeVaryingCacheLineSize(*, memLatency: int, cacheLatency: int, cacheLineNum: int):
    bs = []
    for cacheLineSize in {16, 32, 64, 128}:
        bs += runBenchmarkForMappingType(cacheLineNum=cacheLineNum, memLatency=memLatency, cacheLatency=cacheLatency, cacheLineSize=cacheLineSize)
    return bs

def runBenchmarkForMappingTypeVaryingMemLatency(*, cacheLineSize:int, cacheLatency: int, cacheLineNum: int):
    bs = []
    for memLatency in {1, 10, 100, 1000}:
        bs += runBenchmarkForMappingType(cacheLineNum=cacheLineNum, memLatency=memLatency, cacheLatency=cacheLatency, cacheLineSize=cacheLineSize)
    return bs

def runBenchmarkForMappingTypeVaryingAlg(*, memLatency:int, cacheLineSize:int, cacheLatency: int, cacheLineNum: int):
    bs = []
    for alg in {"merge", "radix"}:
        bs += runBenchmarkForMappingType(alg=alg, cacheLineNum=cacheLineNum, memLatency=memLatency, cacheLatency=cacheLatency, cacheLineSize=cacheLineSize)
    return bs

benches: list[BenchmarkResult] = runBenchmarkForPolicyAndCacheLineNum(memLatency=100, cacheLatency=5, cacheLineSize=16)
printAsCSV(["Policy", "Cacheline-Num" "Gates"], [[b.policy for b in benches], [b.cacheLineNum for b in benches], [b.cacheLineSize for b in benches], [b.result.gates for b in benches]], "mappingBenchmarkPolicyGates.csv")

mergeBenches: list[BenchmarkResult] = runBenchmarkForAlg("merge", cacheLineSize=16, cacheLineNum=8, memLatency=100, cacheLatency=20)
print("done with merge benchess")
radixBenches: list[BenchmarkResult] = runBenchmarkForAlg("radix", cacheLineSize=16, cacheLineNum=8, memLatency=100, cacheLatency=20)

benches = mergeBenches + radixBenches
printAsCSV(["Algorithm", "Size", "Hit-%", "Cycles/M.A."], [[b.alg for b in benches] ,[b.inputSize for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "algBenchmarks.csv")
print("Done with alg")

benches: list[BenchmarkResult] = runBenchmarkForCacheSize(cacheLineNum=8, memLatency=100, cacheLatency=5)
printAsCSV(["Cacheline-Size", "Hit-%", "Cycles/M.A."], [[b.cacheLineSize for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "cacheLineSizeBenchmarksFastMem.csv")
print("Done with cacheline size")

benches: list[BenchmarkResult] = runBenchmarkForCacheSize(cacheLineNum=8, memLatency=500, cacheLatency=5)
printAsCSV(["Cacheline-Size", "Hit-%", "Cycles/M.A."], [[b.cacheLineSize for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "cacheLineSizeBenchmarksSlowMem.csv")
print("done with cacheline size slow")


benches: list[BenchmarkResult] = runBenchmarkForPolicy(cacheLineNum=8, cacheLineSize=16, memLatency=100, cacheLatency=5)
printAsCSV(["Replacement-Policy", "Hit-%", "Cycles/M.A."], [[b.policy for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "policyBenchmarks.csv")
print("done with replacement policy")
    
    
benches: list[BenchmarkResult] = runBenchmarkForMappingType(cacheLineNum=8, cacheLineSize=16, memLatency=100, cacheLatency=5)
printAsCSV(["Mapping-Type", "Hit-%", "Cycles/M.A."], [["Direct" if b.direct_mapped else "Fully-Associative"  for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "mappingBenchmarks.csv")
print("done with mapping type")
    
benches: list[BenchmarkResult] = runBenchmarkForMappingTypeVaryingCacheLineNum(cacheLineSize=16,memLatency=100, cacheLatency=5)
printAsCSV(["Mapping-Type", "Cacheline-Size", "Hit-%", "Cycles/M.A."], [["Direct" if b.direct_mapped else "Fully-Associative"  for b in benches], [b.cacheLineNum for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "mappingBenchmarksLineNum.csv")
print("done withm mapping type and cacheline size")
   
benches: list[BenchmarkResult] = runBenchmarkForMappingTypeVaryingCacheLineSize(cacheLineNum=32,memLatency=100, cacheLatency=5)
printAsCSV(["Mapping-Type", "Cacheline-Num", "Hit-%", "Cycles/M.A."], [["Direct" if b.direct_mapped else "Fully-Associative"  for b in benches], [b.cacheLineSize for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "mappingBenchmarksLineSize.csv")
print("done with mapping type and cacheline num")
   
   
benches: list[BenchmarkResult] = runBenchmarkForMappingTypeVaryingMemLatency(cacheLineNum=32,cacheLineSize=16, cacheLatency=5)
printAsCSV(["Mapping-Type", "Mem-Latency", "Hit-%", "Cycles/M.A."], [["Direct" if b.direct_mapped else "Fully-Associative"  for b in benches], [b.memLatency for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "mappingBenchmarksMemLatency.csv")

   
benches: list[BenchmarkResult] = runBenchmarkForMappingTypeVaryingAlg(memLatency=100, cacheLineNum=32,cacheLineSize=16, cacheLatency=5)
printAsCSV(["Mapping-Type", "Alg", "Hit-%", "Cycles/M.A."], [["Direct" if b.direct_mapped else "Fully-Associative"  for b in benches], [b.alg for b in benches], [100*b.result.hits / (b.result.hits+b.result.misses) for b in benches], [b.result.cyclesNeeded  / ((b.result.hits+b.result.misses)) for b in benches]], "mappingBenchmarksAlg.csv")

benches: list[BenchmarkResult] = runBenchmarkForMappingTypeVaryingCacheLineNumAndCacheLineSize(memLatency=100, cacheLatency=5)
printAsCSV(["Mapping-Type", "Cacheline-Size", "Cacheline-Num", "Gates"], [["Direct" if b.direct_mapped else "Fully-Associative"  for b in benches], [b.cacheLineSize for b in benches], [b.cacheLineNum for b in benches], [b.result.gates for b in benches]], "mappingBenchmarksLineNumGates.csv")

