import unittest
import subprocess
import os


CACHE_PATH = os.path.abspath(os.path.join('..', './cache'))
FILE_PATH = os.path.abspath('..')
INVALID_FILE_PATH = os.path.abspath('InvalidFiles')


def capture_stderr(cmdline_args):
    os.chdir('..')
    capture = subprocess.run(CACHE_PATH + ' ' + cmdline_args, stderr=subprocess.PIPE, stdout=subprocess.PIPE, shell=True)
    return capture.stderr


class TestCheckFile(unittest.TestCase):
    def test_validate_file_format(self):
        args = FILE_PATH + '/src/main.c'
        expected_output = ("Error: " + args + " is not a valid csv file!\n") + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_error_opening(self):
        args = FILE_PATH + '/src/cache'
        expected_output = "Error opening input file: No such file or directory\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_filename_is_directory(self):
        args = FILE_PATH + '/src'
        expected_output = "Error: Filename should not be a directory.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_filename_empty_file(self):
        args = INVALID_FILE_PATH + '/empty.csv'
        expected_output = "Error: Input file contains no data.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)


class TestExtractFileData(unittest.TestCase):
    def test_error_reading_from_file(self):
        args = INVALID_FILE_PATH + '/invalid_data.csv'
        expected_output = "Error: Wrong file format! First column is not set right!\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_no_addres_given(self):
        args = INVALID_FILE_PATH + '/no_address.csv'
        expected_output = "Error: Wrong file format! No address given.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_no_data_saved(self):
        args = INVALID_FILE_PATH + '/no_data_saved_w.csv'
        expected_output = "Error: Wrong file format! No data saved.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_no_empty_data_when_reading(self):
        args = INVALID_FILE_PATH + '/no_empty_data_R.csv'
        expected_output = "Error: Wrong file format! When reading from a file, data should be empty.\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)

    def test_no_valid_we(self):
        args = INVALID_FILE_PATH + '/no_valid_we.csv'
        expected_output = "Error: 'X' is not a valid operation in input file\n" + print_usage
        output = capture_stderr(args).decode()
        self.assertEqual(expected_output, output)


if __name__ == '__main__':
    unittest.main()

print_usage = ("usage: /u/halle/brieg/home_at/GRA_Project/gra24cdaproject-g127/cache [-c c/--cycles c] [--lcycles] "
               "[--directmapped] [--fullassociative] "
               "[--cacheline-size s] [--cachelines n] [--cache-latency l] [--memorylatency m] "
               "[--lru] [--fifo] [--random] [--use-cache=<Y,n>] [--tf=<filename>] [--extended] [-h/--help] <filename>\n"
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
