import random
import string

# Parameters to change command line argument size and line count
flags = 5
lines = 15

progname = "./cache "
pos_argument = "examples/exampleinputfile.csv "  # valid file
# pos_argument = random.choice(string)    # invalid argument

choices = [0, "tests/validTracefile.trace", "invalidTracefile.trace"]

# Test file with valid flags
valid_input = ["--cycles ", "-c ",
               "--directmapped ", "--fullassociative ",
               "--cacheline-size ", "--cachelines ",
               "--cache-latency ", "--memory-latency ",
               "--tf=", "-h ", "--help ",
               "--lru ", "--fifo ", "--random "
               ]

# Fuzz file with random string inputs
random_input = []
for i in range(0, flags):
    argument = str(random.choice(["-", "--"]))
    for j in range(0, random.randint(1, 30)):
        argument += str(random.choice([random.choice(string.ascii_letters), random.choice(string.digits),
                                       random.choice(string.hexdigits)]))
    random_input.append(argument + "\t")

# Create files
file_name = "testValidCommandLineArguments.sh"
filename = "invalidFileCommandLineTest.sh"

# Write x random "valid" command lines into file
file = open(file_name, "w")
for i in range(0, lines):
    arguments = ""

    for j in range(0, flags):  # Add x flags to command line input
        choice = random.choice(choices)
        if choice == 0:
            choice = random.randint(1, 205_000_000)
        arguments += str(random.choice(valid_input)) + str(choice) + "\t"

    line = progname + pos_argument + arguments
    file.write(line + "\n")

file.close()

# Write x random command lines into file
file = open(filename, "w")
for i in range(0, lines):
    arguments = ""

    for j in range(0, flags):  # Add x flags to command line input
        choice = random.choice(choices)
        if choice == 0:
            choice = random.randint(-30*20*393, 300*5849*33)
        arguments += str(random.choice(random_input)) + str(choice) + "\t"

    line = progname + pos_argument + arguments
    file.write(line + "\n")

file.close()
