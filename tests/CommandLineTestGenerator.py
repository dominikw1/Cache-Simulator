import random

# Parameters to change command line argument size and line count
flags = 15
lines = 15

progname = "./cache "
pos_argument = "examples/exampleinputfile.csv "  # valid file
valid_input = ["--cycles ", "-c ",
               "--directmapped ", "--fullassociative ",
               "--cacheline-size ", "--cachelines ",
               "--cache-latency ", "--memory-latency ",
               # "-h ", "--help ",
               "--tf=tests/validTracefile.trace "
               "--lru ", "--fifo ", "--random ", "--use-cache=no "
               ]

choices = [0, -1]
num_choices = [0, 16, 4, 32, 64, 128, 256, 1024, 2048, random.randint(-2_400, 2_500_000)]

valgrind_test = "valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes "


# Create files
file_name = "testValidCommandLineArguments.sh"

# Write x random "valid" command lines into file
file = open(file_name, "w")
for i in range(0, lines):
    arguments = ""

    for j in range(0, random.randint(0, flags)):  # Add x flags to command line input
        choice = random.choice(choices)
        if choice == 0:
            choice = random.choice(num_choices)
            arguments += str(random.choice(valid_input)) + str(choice) + " "
        else:
            arguments += str(random.choice(valid_input)) + " "

    line = progname + pos_argument + arguments
    file.write("echo input: " + pos_argument + arguments + "\n" + valgrind_test + line + "\n")

file.close()
