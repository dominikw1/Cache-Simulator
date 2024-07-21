echo "Hello World"
make
echo input: examples/merge_sort_10.csv --fifo  --cycles  examples/merge_sort_10.csv  -c  examples/merge_sort_10.csv  --cache-latency 64 -c  --random 128
./cache examples/merge_sort_10.csv --fifo  --cycles  examples/merge_sort_10.csv  -c  examples/merge_sort_10.csv  --cache-latency 64 -c  --random 128
echo input: examples/merge_sort_10.csv --cachelines 1024 --cachelines 2010427 --fullassociative  examples/merge_sort_10.csv 2048 examples/merge_sort_10.csv 2048 --cycles
./cache examples/merge_sort_10.csv --cachelines 1024 --cachelines 2010427 --fullassociative  examples/merge_sort_10.csv 2048 examples/merge_sort_10.csv 2048 --cycles
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv 32 --tf--lru 4 --memory-latency  examples/merge_sort_10.csv  examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv 32 --tf--lru 4 --memory-latency  examples/merge_sort_10.csv  examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv --cache-latency  --tf--lru 4
./cache examples/merge_sort_10.csv --cache-latency  --tf--lru 4
echo input: examples/merge_sort_10.csv --cycles 16 examples/merge_sort_10.csv 128 --help  --cycles  --directmapped 4
./cache examples/merge_sort_10.csv --cycles 16 examples/merge_sort_10.csv 128 --help  --cycles  --directmapped 4
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv  --cachelines 128
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv  --cachelines 128
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv  --cycles 64
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv  --cycles 64
echo input: examples/merge_sort_10.csv -c 1024
./cache examples/merge_sort_10.csv -c 1024
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv --random  examples/merge_sort_10.csv  -c 256 examples/merge_sort_10.csv  examples/merge_sort_10.csv  --use-cache no  --use-cache no 128
./cache examples/merge_sort_10.csv --random  examples/merge_sort_10.csv  -c 256 examples/merge_sort_10.csv  examples/merge_sort_10.csv  --use-cache no  --use-cache no 128
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv 16 --fifo  --random  --cacheline-size  --random 1024 --cachelines  --cacheline-size  --use-cache no 32
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv 16 --fifo  --random  --cacheline-size  --random 1024 --cachelines  --cacheline-size  --use-cache no 32
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv 128
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv 128
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv 2010427 --tf--lru 0 examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv 2010427 --tf--lru 0 examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv  --fullassociative 4 --cacheline-size 32 examples/merge_sort_10.csv  examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv  --fullassociative 4 --cacheline-size 32 examples/merge_sort_10.csv  examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv --cycles  --random  -c 2048
./cache examples/merge_sort_10.csv --cycles  --random  -c 2048
echo input: examples/merge_sort_10.csv --tf--lru  examples/merge_sort_10.csv 256 --directmapped 2048 --fullassociative 0 --fullassociative 0 --cacheline-size 64 examples/merge_sort_10.csv 128
./cache examples/merge_sort_10.csv --tf--lru  examples/merge_sort_10.csv 256 --directmapped 2048 --fullassociative 0 --fullassociative 0 --cacheline-size 64 examples/merge_sort_10.csv 128
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv 2010427
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv 2010427
echo input: examples/merge_sort_10.csv --cachelines 256 --fifo 128
./cache examples/merge_sort_10.csv --cachelines 256 --fifo 128
echo input: examples/merge_sort_10.csv --fifo  --cache-latency  -c  examples/merge_sort_10.csv  --cycles 32 --random
./cache examples/merge_sort_10.csv --fifo  --cache-latency  -c  examples/merge_sort_10.csv  --cycles 32 --random
echo input: examples/merge_sort_10.csv --cacheline-size  --help 256 examples/merge_sort_10.csv  examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv --cacheline-size  --help 256 examples/merge_sort_10.csv  examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv 0 -h 4 --fifo  --fifo 128
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv 0 -h 4 --fifo  --fifo 128
echo input: examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv --random  --use-cache no  examples/merge_sort_10.csv  --cycles  --help  examples/merge_sort_10.csv  --memory-latency 32 -h  --fifo 256
./cache examples/merge_sort_10.csv --random  --use-cache no  examples/merge_sort_10.csv  --cycles  --help  examples/merge_sort_10.csv  --memory-latency 32 -h  --fifo 256
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv 256 examples/merge_sort_10.csv  examples/merge_sort_10.csv  --directmapped 1024 --cacheline-size  --cache-latency  examples/merge_sort_10.csv 64 examples/merge_sort_10.csv 32
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv 256 examples/merge_sort_10.csv  examples/merge_sort_10.csv  --directmapped 1024 --cacheline-size  --cache-latency  examples/merge_sort_10.csv 64 examples/merge_sort_10.csv 32
echo input: examples/merge_sort_10.csv --cachelines 2048 --fifo 1024 --tf--lru 1024 examples/merge_sort_10.csv  --fullassociative  examples/merge_sort_10.csv 32 examples/merge_sort_10.csv  examples/merge_sort_10.csv 64
./cache examples/merge_sort_10.csv --cachelines 2048 --fifo 1024 --tf--lru 1024 examples/merge_sort_10.csv  --fullassociative  examples/merge_sort_10.csv 32 examples/merge_sort_10.csv  examples/merge_sort_10.csv 64
echo input: examples/merge_sort_10.csv -c  examples/merge_sort_10.csv 1024 --directmapped  --random 128 --fifo 1024 --fullassociative  examples/merge_sort_10.csv 256 --cacheline-size 32 --cachelines 1024 examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv -c  examples/merge_sort_10.csv 1024 --directmapped  --random 128 --fifo 1024 --fullassociative  examples/merge_sort_10.csv 256 --cacheline-size 32 --cachelines 1024 examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv -c 0 examples/merge_sort_10.csv  examples/merge_sort_10.csv  examples/merge_sort_10.csv 32 --memory-latency  examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv -c 0 examples/merge_sort_10.csv  examples/merge_sort_10.csv  examples/merge_sort_10.csv 32 --memory-latency  examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv --use-cache no  -c  --cache-latency 256
./cache examples/merge_sort_10.csv --use-cache no  -c  --cache-latency 256
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv 32 --memory-latency 2048 examples/merge_sort_10.csv 128 --tf--lru  --directmapped 128 examples/merge_sort_10.csv  --cache-latency 16 --cachelines 4 examples/merge_sort_10.csv 1024 examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv 32 --memory-latency 2048 examples/merge_sort_10.csv 128 --tf--lru  --directmapped 128 examples/merge_sort_10.csv  --cache-latency 16 --cachelines 4 examples/merge_sort_10.csv 1024 examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv -c 0 examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv -c 0 examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv  --random  -c 0 examples/merge_sort_10.csv  examples/merge_sort_10.csv 0
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv  --random  -c 0 examples/merge_sort_10.csv  examples/merge_sort_10.csv 0
echo input: examples/merge_sort_10.csv --cachelines  --tf--lru 4 --cycles 64 --fifo  examples/merge_sort_10.csv  -h 64 --cache-latency  -c  examples/merge_sort_10.csv 64
./cache examples/merge_sort_10.csv --cachelines  --tf--lru 4 --cycles 64 --fifo  examples/merge_sort_10.csv  -h 64 --cache-latency  -c  examples/merge_sort_10.csv 64
echo input: examples/merge_sort_10.csv --random 16 -h  --help 256 --memory-latency  --cache-latency  --random 2010427 -h 16 --fifo
./cache examples/merge_sort_10.csv --random 16 -h  --help 256 --memory-latency  --cache-latency  --random 2010427 -h 16 --fifo
echo input: examples/merge_sort_10.csv --tf--lru  examples/merge_sort_10.csv 2010427 examples/merge_sort_10.csv 128 --cachelines 2048 -c  examples/merge_sort_10.csv 1024 --use-cache no  examples/merge_sort_10.csv 1024
./cache examples/merge_sort_10.csv --tf--lru  examples/merge_sort_10.csv 2010427 examples/merge_sort_10.csv 128 --cachelines 2048 -c  examples/merge_sort_10.csv 1024 --use-cache no  examples/merge_sort_10.csv 1024
echo input: examples/merge_sort_10.csv --directmapped  --cacheline-size 1024 examples/merge_sort_10.csv 1024 --tf--lru  --directmapped  examples/merge_sort_10.csv  --cache-latency  examples/merge_sort_10.csv  --memory-latency 64 --fifo 2010427
./cache examples/merge_sort_10.csv --directmapped  --cacheline-size 1024 examples/merge_sort_10.csv 1024 --tf--lru  --directmapped  examples/merge_sort_10.csv  --cache-latency  examples/merge_sort_10.csv  --memory-latency 64 --fifo 2010427
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv  --help 32
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv  --help 32
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv 256 examples/merge_sort_10.csv  examples/merge_sort_10.csv  -c
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv 256 examples/merge_sort_10.csv  examples/merge_sort_10.csv  -c
echo input: examples/merge_sort_10.csv --cacheline-size 16 examples/merge_sort_10.csv 32
./cache examples/merge_sort_10.csv --cacheline-size 16 examples/merge_sort_10.csv 32
echo input: examples/merge_sort_10.csv --directmapped 32 --memory-latency 1024 examples/merge_sort_10.csv 16 --tf--lru 32 examples/merge_sort_10.csv  -c
./cache examples/merge_sort_10.csv --directmapped 32 --memory-latency 1024 examples/merge_sort_10.csv 16 --tf--lru 32 examples/merge_sort_10.csv  -c
echo input: examples/merge_sort_10.csv --tf--lru 4 --use-cache no 2010427 -c 128 --fifo  examples/merge_sort_10.csv  examples/merge_sort_10.csv  --directmapped
./cache examples/merge_sort_10.csv --tf--lru 4 --use-cache no 2010427 -c 128 --fifo  examples/merge_sort_10.csv  examples/merge_sort_10.csv  --directmapped
echo input: examples/merge_sort_10.csv --cacheline-size 32 --help 4 --fifo 64 --cycles 128 examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv --cacheline-size 32 --help 4 --fifo 64 --cycles 128 examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv  --cachelines 32 examples/merge_sort_10.csv  -c 16 --cachelines 0 examples/merge_sort_10.csv 2048 --memory-latency 16 --cycles 2010427 --cycles
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv  --cachelines 32 examples/merge_sort_10.csv  -c 16 --cachelines 0 examples/merge_sort_10.csv 2048 --memory-latency 16 --cycles 2010427 --cycles
echo input: examples/merge_sort_10.csv --cachelines  --use-cache no  --use-cache no  examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv --cachelines  --use-cache no  --use-cache no  examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv  -c  --cycles
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv  -c  --cycles
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv 2010427 --tf--lru  examples/merge_sort_10.csv  examples/merge_sort_10.csv 64
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv 2010427 --tf--lru  examples/merge_sort_10.csv  examples/merge_sort_10.csv 64
echo input: examples/merge_sort_10.csv --directmapped  examples/merge_sort_10.csv 128 --fifo 2010427 examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv --directmapped  examples/merge_sort_10.csv 128 --fifo 2010427 examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv --cacheline-size 2010427 --use-cache no 4 examples/merge_sort_10.csv  examples/merge_sort_10.csv 1024 examples/merge_sort_10.csv
./cache examples/merge_sort_10.csv --cacheline-size 2010427 --use-cache no 4 examples/merge_sort_10.csv  examples/merge_sort_10.csv 1024 examples/merge_sort_10.csv
echo input: examples/merge_sort_10.csv --cacheline-size
./cache examples/merge_sort_10.csv --cacheline-size
echo input: examples/merge_sort_10.csv examples/merge_sort_10.csv  --cycles 32 --memory-latency  --cacheline-size 4
./cache examples/merge_sort_10.csv examples/merge_sort_10.csv  --cycles 32 --memory-latency  --cacheline-size 4
