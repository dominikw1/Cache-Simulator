import unittest
import subprocess
import os

CACHE_PATH = os.path.abspath(os.path.join('..', './cache'))
FILE_PATH = os.path.abspath(os.path.join('..', 'examples', 'exampleinputfile.csv'))


def capture_stderr(cmdline_args):
    os.chdir('..')
    capture = subprocess.run(CACHE_PATH + cmdline_args, stderr=subprocess.PIPE, stdout=subprocess.PIPE, shell=True)
    return capture.stderr


class TestInvalidInput(unittest.TestCase):
    def test_missing_positional_argument(self):
        args = ''
        expected_output = "Positional argument missing!\n" + print_usage
        output = capture_stderr(args).decode()

        self.assertEqual(expected_output, output)

    def test_no_number_argument(self):
        args = ' -c o ' + FILE_PATH
        expected_output = "Invalid input: 'o' is not a number.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_larger_unsigned_int(self):
        args = ' --cycles 5000000000 ' + FILE_PATH
        expected_output = "Invalid input: 5000000000 is too big to be converted to an unsigned int.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_negative_input(self):
        pass

    # TODO tracefile tests

    def test_no_valid_argument(self):
        pass

    def test_missing_argument(self):
        pass

    def test_not_power_of_sixteen(self):
        pass

    # TODO: Invalid input test main l. 102

    # TODO: Error parsing number l. 106

    # TODO: check cycle size


class TestFile(unittest.TestCase):
    def test_error_opening(self):
        pass

    def test_error_determining_file_size(self):
        # TODO l. 138
        pass

    def test_not_a_regular_file(self):
        pass

    def test_filename_is_directory(self):
        pass

    def test_wrong_file_format(self):
        pass

    def test_hex_numbers(self):
        pass

    def test_no_address_given(self):
        pass

    def test_no_data_saved(self):
        pass

    def test_not_empty_data(self):
        pass

    def test_invalid_operation(self):
        pass

    def test_error_reading_from_file(self):
        pass

    def test_two_policies_set(self):
        pass


class TestWarnings(unittest.TestCase):
    def test_setting_useCache(self):
        args = ' --cachelines 0 ' + FILE_PATH
        expected_output = "Warning: --cachelines must be at least 1. Setting use-cache=n."
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)

    def test_multiple_architecture_inputs(self):
        args = ' --directmapped --fullassociative ' + FILE_PATH
        expected_output = ("Warning: --fullassociative and --directmapped are both set. Using default value "
                           "fullassociative!")
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
        args = ' --use-cache ' + FILE_PATH
        expected_output = "Warning: Option --use-cache is not set. Using default value 'Y'."
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)

    def test_usecache_invalid_option(self):
        args = ' --use-cache=u ' + FILE_PATH
        expected_output = "Warning: Not a valid option for --use-cache. Using default value 'Y'."
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)

    def test_memorylatency_cachelatency(self):
        args = ' --memory-latency 5 --cache-latency 10 ' + FILE_PATH
        expected_output = "Warning: Memory latency is less than cache latency."
        output = capture_stderr(args).decode()
        self.assertIn(expected_output, output)


class TestOther(unittest.TestCase):
    def test_help_message(self):
        pass

    def test_allocating_buffer(self):
        # TODO for requests array
        pass


if __name__ == '__main__':
    unittest.main()

print_usage = ("usage: /u/halle/brieg/home_at/GRA_Project/gra24cdaproject-g127/cache <filename> [-c c/--cycles c] "
               "[--lcycles] [--directmapped] [--fullassociative] "
               "[--cacheline-size s] [--cachelines n] [--cache-latency l] [--memorylatency m] "
               "[--lru] [--fifo] [--random] [--use-cache=<Y,n>] [--tf=<filename>] [--extended] [-h/--help]\n"
               "   -c c / --cycles c       Set the number of cycles to be simulated to c. Allows inputs in range "
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
               "   --tf=<filename>         File name for a trace file containing all signals. If not set, no "
               "trace file will be created\n"
               "   -h / --help             Show help message and exit\n")
