import unittest
import subprocess
import os

CACHE_PATH = os.path.abspath(os.path.join('..', './cache'))
FILE_PATH = ' ' + os.path.abspath(os.path.join('..', 'examples', 'merge_sort_10.csv'))
PATH_TO_EXISTING_FILE = os.path.abspath(os.path.join('..', 'tests', 'FileProcessorTests.py'))
PATH_TO_DIRECTORY = os.path.abspath(os.path.join('..', 'tests'))


def capture_stderr(cmdline_args):
    os.chdir('..')
    capture = subprocess.run(CACHE_PATH + cmdline_args, stderr=subprocess.PIPE, stdout=subprocess.PIPE, shell=True)
    return capture.stderr


class TestInvalidInput(unittest.TestCase):
    def test_missing_positional_argument(self):
        args = ''
        expected_output = "Error: Positional argument missing!\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_no_number_argument(self):
        args = ' -c o ' + FILE_PATH
        expected_output = "Invalid input: 'o' is not an int.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_larger_unsigned_int(self):
        args = ' --cycles 9000000000 ' + FILE_PATH
        expected_output = "Invalid input: 9000000000 is too big to be converted to an unsigned int.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_invalid_unsigned_int(self):
        args = ' --cycles 5,000 ' + FILE_PATH
        expected_output = "Invalid input: '5,000' is not an int.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_invalid_cycles(self):
        args = ' --cycles -50000 ' + FILE_PATH
        expected_output = "Invalid input: Cycles cannot be smaller than 1.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_invalid_cacheline_size(self):
        args = ' --cacheline-size -50000 ' + FILE_PATH
        expected_output = "Invalid input: Cacheline size should be at least 1.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_multiple_of_sixteen(self):
        args = ' --cacheline-size 8 ' + FILE_PATH
        expected_output = "Invalid input: Cacheline size should be a multiple of 16 bytes!\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_power_of_two(self):
        args = ' --cacheline-size 96 ' + FILE_PATH
        expected_output = "Invalid input: Cacheline size should be a power of 2!\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_invalid_num_of_cachelines(self):
        args = ' --cachelines -50000 ' + FILE_PATH
        expected_output = "Invalid input: Number of cache-lines must be at least 1.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_invalid_cache_latency(self):
        args = ' --cache-latency -50 ' + FILE_PATH
        expected_output = "Invalid input: Cache-latency cannot be negative.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_invalid_memory_latency(self):
        args = ' --memory-latency -50 ' + FILE_PATH
        expected_output = "Invalid input: Memory-latency cannot be negative.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_multiple_policy_input(self):
        args = ' --fifo --random ' + FILE_PATH
        expected_output = "Error: --fifo and --random are both set. Please choose only one option!\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_missing_argument_use_cache_1(self):
        args = ' --use-cache= ' + FILE_PATH
        expected_output = "Error: Option --use-cache requires an argument.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_invalid_argument_use_cache(self):
        args = ' --use-cache x ' + FILE_PATH
        expected_output = "Error: 'x' is not a valid option for --use-cache.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_missing_argument_trace_file_1(self):
        args = ' --tf= ' + FILE_PATH
        expected_output = "Error: Option --tf requires an argument.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_existing_directory_tf(self):
        args = FILE_PATH + ' --tf ' + PATH_TO_DIRECTORY
        expected_output = "Error: Filename '" + PATH_TO_DIRECTORY + ("' is a directory name. Please choose a different"
                                                                     " filename for the tracefile.\n") + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_existing_file_tf(self):
        args = FILE_PATH + ' --tf ' + PATH_TO_EXISTING_FILE
        expected_output = "Error: File '" + PATH_TO_EXISTING_FILE + ("' already exists. Please choose a different "
                                                                     "filename for the tracefile.\n") + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_missing_argument_cycles(self):
        args = FILE_PATH + ' --cycles '
        expected_output = "Error: Option -c/--cycles requires an argument.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_missing_argument_cacheline_size(self):
        args = FILE_PATH + ' --cacheline-size '
        expected_output = "Error: Option --cacheline-size requires an argument.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_missing_argument_cachelines(self):
        args = FILE_PATH + ' --cachelines '
        expected_output = "Error: Option --cachelines requires an argument.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_missing_argument_cache_latency(self):
        args = FILE_PATH + ' --cache-latency '
        expected_output = "Error: Option --cache-latency requires an argument.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_missing_argument_memory_latency(self):
        args = FILE_PATH + ' --memory-latency '
        expected_output = "Error: Option --memory-latency requires an argument.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_missing_argument_use_cache_2(self):
        args = FILE_PATH + ' --use-cache '
        expected_output = "Error: Option --use-cache requires an argument.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_missing_argument_trace_file_2(self):
        args = FILE_PATH + ' --tf '
        expected_output = "Error: Option --tf requires an argument.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_no_valid_option(self):
        args = ' --xyz ' + FILE_PATH
        expected_output = "Error: Not a valid option '--xyz'.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_missing_posargument(self):
        args = ' --directmapped --cycles 10 '
        expected_output = "Error: Positional argument is missing!\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)


class TestWarnings(unittest.TestCase):
    def test_setting_useCache(self):
        args = ' --cachelines 0 ' + FILE_PATH
        expected_output = "Warning: --cachelines must be at least 1. Setting use-cache=n and --extended."
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)

    def test_multiple_architecture_inputs(self):
        args = ' --directmapped --fullassociative ' + FILE_PATH
        expected_output = ("Warning: --fullassociative and --directmapped are both set. Using default value "
                           "fullassociative!")
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)

    def test_multiple_policy_inputs(self):
        args = ' --lru --fifo ' + FILE_PATH
        expected_output = "Warning: More than one policy set. Simulating cache using default value LRU!\n"
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)

    def test_multiple_of_two(self):
        args = ' --cachelines 18 ' + FILE_PATH
        expected_output = "Warning: Number of cachelines are usually a power of two!"
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)

    def test_multiple_policies(self):
        args = ' --lru --fifo ' + FILE_PATH
        expected_output = "Warning: More than one policy set. Simulating cache using default value LRU!"
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)

    def test_usecache_not_set(self):
        args = ' --use-cache= ' + FILE_PATH
        expected_output = "Error: Option --use-cache requires an argument.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)

    def test_usecache_invalid_option(self):
        args = ' --use-cache=u ' + FILE_PATH
        expected_output = "Error: 'u' is not a valid option for --use-cache.\n"
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)

    def test_memorylatency_cachelatency(self):
        args = ' --memory-latency 5 --cache-latency 10 ' + FILE_PATH
        expected_output = "Warning: Memory latency is less than cache latency."
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)

    def test_check_cycle_size(self):
        args = ' --cycles 4000000000 ' + FILE_PATH
        expected_output = ("Error: Overflow detected. Input for -c/--cycles is too big to be converted "
                           "to an int. Set option --lcycles to increase range.\n") + print_usage
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)

    def test_zero_cache_latency(self):
        args = ' --cache-latency 0 ' + FILE_PATH
        expected_output = "Warning: A value of 0 for --cache-latency is not realistic!\n"
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_zero_memory_latency(self):
        args = ' --memory-latency 0 ' + FILE_PATH
        expected_output = ("Warning: A value of 0 for --memory-latency is not realistic!\nWarning: Memory latency is "
                           "less than cache latency.\n")
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)


class TestOther(unittest.TestCase):
    def test_help_message(self):
        args = ' --help ' + FILE_PATH
        expected_output = (print_usage + "\n"
                           + ("Positional arguments:\n"
                              "   <filename>   The name of the file to be processed\n"
                              "\n"
                              "Optional arguments:\n"
                              "   -c c / --cycles c       The number of cycles used for the simulation "
                              "(default: c = 100000)\n"
                              "   --lcycles               If set input for cycles of up to 2^32-1 are allowed\n"
                              "   --directmapped          Simulates a direct-mapped cache\n"
                              "   --fullassociative       Simulates a fully associative cache (Set as default)\n"
                              "   --cacheline-size s      The size of a cache line in bytes (default: 64)\n"
                              "   --cachelines n          The number of cache lines (default: 256)\n"
                              "   --cache-latency l       The cache latency in cycles (default: 2)\n"
                              "   --memory-latency m      The memory latency in cycles (default: 100)\n"
                              "   --lru                   Use LRU as cache-replacement policy (Set as default)\n"
                              "   --fifo                  Use FIFO as cache-replacement policy\n"
                              "   --random                Use random cache-replacement policy\n"
                              "   --use-cache=<Y,n>       Simulates a system with cache or no cache (default: Y)\n"
                              "   --tf=<filename>         The name for a trace file containing all signals. "
                              "If not set, no "
                              "trace file will be created\n"
                              "   --extended              Calls extended run_simulation-method with additional "
                              "parameters "
                              "\'policy' and \'use-cache'\n"
                              "   -h / --help             Show this help message and exit\n"))
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)


if __name__ == '__main__':
    unittest.main()

print_usage = ("usage: " + CACHE_PATH + " [-c c/--cycles c] [--lcycles] "
                                        "[--directmapped] [--fullassociative] "
                                        "[--cacheline-size s] [--cachelines n] [--cache-latency l] [--memorylatency m] "
                                        "[--lru] [--fifo] [--random] [--use-cache=<Y,n>] [--tf=<filename>] [--extended]"
                                        " [-h/--help] <filename>\n"
                                        "   -c c / --cycles c       Set the number of cycles to be simulated to c. "
                                        "Allows inputs in range "
                                        "[0,2^16-1]\n"
                                        "   --lcycles               Allow input of cycles of up to 2^32-1\n"
                                        "   --directmapped          Simulate a direct-mapped cache\n"
                                        "   --fullassociative       Simulate a fully associative cache\n"
                                        "   --cacheline-size s      Set the cache line size to s bytes\n"
                                        "   --cachelines n          Set the number of cachelines to n\n"
                                        "   --cache-latency l       Set the cache latency to l cycles\n"
                                        "   --memory-latency m      Set the memory latency to m cycles\n"
                                        "   --lru                   Use LRU as cache-replacement policy\n"
                                        "   --fifo                  Use FIFO as cache-replacement policy\n"
                                        "   --random                Use random cache-replacement policy\n"
                                        "   --use-cache=<Y,n>       Simulates a system with cache or no cache\n"
                                        "   --extended              Call extended run_simulation-method\n"
                                        "   --tf=<filename>         File name for a trace file containing all signals."
                                        " If not set, no "
                                        "trace file will be created\n"
                                        "   -h / --help             Show help message and exit\n")
