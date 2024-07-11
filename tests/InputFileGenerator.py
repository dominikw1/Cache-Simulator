import random
import string

numOfFiles = 10
numOfLines = 20

valid_input = ["R", "r", "W", "w", "", ""]
random_input = valid_input + list(random.choice(string.ascii_letters)) + list(random.choice(string.ascii_letters))

# Create 10 new random files
for i in range(1, numOfFiles):
    file_name = "invalidInputFile" + str(i) + ".csv"
    file = open(file_name, "w")

    for j in range(0, numOfLines):
        line = (random.choice(random_input) + "," + str(random.randint(1, 205_000_000)) + "," +
                random.choice([(str(hex(random.randint(1, 205_000_000)))), ""]))

        file.write(line + "\n")

    file.close()

# TODO: Create 10 valid files
