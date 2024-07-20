#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "Argparsing.h"
#include "FileDataExtraction.h"
#include "Request.h"

int extract_file_data(const char* progname, FILE* file, struct Request* requests, size_t* numRequests) {
    // Check input file and save file data to requests
    // Inspired by: https://github.com/portfoliocourses/c-example-code/blob/main/csv_to_struct_array.c
    int read_line;

    do {
        char we;
        uint32_t addr;
        uint32_t data;

        read_line = fscanf(file, "%c,%i,%i\n", &we, &addr, &data);

        if (read_line < 3 && !feof(file)) {
            if (read_line == -1) {
                fprintf(stderr, "Wrong file format! An error occurred while reading from the file.\n");
            }
            if (read_line < 2) { // Address was not read from file
                fprintf(stderr, "Wrong file format! No address given.\n");
                goto error;
            }
        }

        if ((we == 'W' || we == 'w') && read_line < 3) { // Write should have data written in the file
            fprintf(stderr, "Wrong file format! No data saved.\n");
            goto error;
        }
        if ((we == 'R' || we == 'r') && read_line == 3) { // Read should not have data written in the file
            fprintf(stderr, "Wrong file format! When reading from a file, data should be empty.\n");
            goto error;
        }

        requests[*numRequests].addr = addr;
        requests[*numRequests].data = data;

        if (we == 'R' || we == 'r') {
            requests[*numRequests].we = 0;
        } else if (we == 'W' || we == 'w') {
            requests[*numRequests].we = 1;
        } else {
            fprintf(stderr, "'%c' is not a valid operation\n", we);
            goto error;
        }

        (*numRequests)++;

        if (ferror(file)) {
            fprintf(stderr, "Error reading from file. Error code: %d\n", ferror(file));
            goto error;
        }

    } while (!feof(file));

    fclose(file);
    return EXIT_SUCCESS;

error:
    fclose(file);
    print_usage(progname);
    free(requests);
    requests = NULL;
    exit(EXIT_FAILURE);
}
