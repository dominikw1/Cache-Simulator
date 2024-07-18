from pathlib import Path
import subprocess

pathToCacheExe=  "../cache"

def runBenchmark(input: Path, cacheSize: int, cacheLineNum: int, memLatency: int, cacheLatency: int) -> str:
    proc = subprocess.Popen([pathToCacheExe, input,"lcycles", "-c", str(6553500), "--cache-latency" ,str(cacheLatency), "--memory-latency",str(memLatency) ,"--cachelines",str(cacheLineNum), "--cacheline-size", str(cacheSize) ], stdout=subprocess.PIPE)
    return proc.communicate()[0]

def runBenchmarkForAlg(alg: str, *, cacheSize: int, cacheLineNum: int, memLatency: int, cacheLatency: int)-> str:
    #print(f"BenchmarkInputGenerator/Benchmarks/{alg}_sort_{10}.csv")
    return (runBenchmark(f"BenchmarkInputGenerator/Benchmarks/{alg}_sort_{10}.csv", cacheSize, cacheLineNum, memLatency, cacheLatency).decode("utf-8") ,runBenchmark(f"BenchmarkInputGenerator/Benchmarks/{alg}_sort_{100}.csv", cacheSize, cacheLineNum, memLatency, cacheLatency).decode("utf-8") ,runBenchmark(f"BenchmarkInputGenerator/Benchmarks/{alg}_sort_{1000}.csv", cacheSize, cacheLineNum, memLatency, cacheLatency).decode("utf-8"))

mergeBenches = runBenchmarkForAlg("merge", cacheSize=64, cacheLineNum=16, memLatency=5, cacheLatency=1)
radixBenches = runBenchmarkForAlg("radix", cacheSize=64, cacheLineNum=16, memLatency=5, cacheLatency=1)

print("Merge sorts:")
for b in mergeBenches:
    print(b)

print("Radix sorts:")
for b in radixBenches:
    print(b)