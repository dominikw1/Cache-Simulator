all:
	make -C RandomNumberGenerator
	# requires LLVM which is not installed on "Rechnerhalle"
	# make -C MemoryAnalyser
	make -C BenchmarkInputGenerator

clean:
	rm -rf */*.out
	rm -rf */randomNumbers_*.txt