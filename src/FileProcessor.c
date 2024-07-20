#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Argparsing.h"
#include "FileProcessor.h"
#include "Request.h"

void validate_file_format(const char* progname, FILE* file, const char* filename, const char* filetype) {
    // Taken and adapted from https://stackoverflow.com/questions/5309471/getting-file-extension-in-c
    const char *dot = strrchr(filename, '.');   // Check for valid file format
    if (dot == NULL || dot == filename) {
        fprintf(stderr, "Error: %s is not a valid file\n", filename);
        fclose(file);
        print_usage(progname);
        exit(EXIT_FAILURE);
    } else if (strcmp(dot + 1, filetype) != 0) {
        fprintf(stderr, "Error: %s is not a valid %s file!\n", filename, filetype);
        fclose(file);
        print_usage(progname);
        exit(EXIT_FAILURE);
    }
}

FILE* check_file(const char* progname, const char* filename_1, const char* filename_2) {
    struct stat file_info;
    const char* filename = filename_1;
    FILE* file = fopen(filename, "r");
    if (file == NULL) { // Accept positional argument as first and last command line argument
        file = fopen(filename_2, "r");
        filename = filename_2;
    }

    if (file == NULL) {
        fprintf(stderr, "Error opening input file: %s\n", strerror(errno));
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    if (fstat(fileno(file), &file_info) != 0) {
        perror("Error determining file size");
        goto error;
    }

    if (S_ISDIR(file_info.st_mode)) {
        fprintf(stderr, "Filename should not be a directory.\n");
        goto error;
    }
    if (file_info.st_size == 0) {
        fprintf(stderr, "Error: Input file contains no data.\n");
        goto error;
    }

    validate_file_format(progname, file, filename, "csv"); // Check if file is csv file

    if (filename != NULL && !S_ISREG(file_info.st_mode)) {
        fprintf(stderr, "Error: %s is not a regular file\n", filename);
        goto error;
    }

    return file;

error:
    fclose(file);
    print_usage(progname);
    exit(EXIT_FAILURE);
}

// Inspired by: https://github.com/portfoliocourses/c-example-code/blob/main/csv_to_struct_array.c
void extract_file_data(const char* progname, FILE* file, struct Configuration* config) {
    struct stat file_info;
    if (fstat(fileno(file), &file_info) != 0) {
        perror("Error determining file size");
        fclose(file);
        print_usage(progname);
        exit(EXIT_FAILURE);
    }
    // Allocate heap-space for requests
    config->requests = (struct Request*)malloc(sizeof(struct Request) * file_info.st_size);
    config->numRequests = 0;
    if (config->requests == NULL) {
        perror("Error allocating memory buffer for file");
        fclose(file);
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    // Check input file and save file data to requests
    int read_line;

    do {
        char we;
        uint32_t addr;
        uint32_t data;

        read_line = fscanf(file, "%c,%i,%i\n", &we, &addr, &data);

        if ((read_line < 3 || read_line > 3) && !feof(file)) {
            if (read_line == -1) {
                fprintf(stderr, "Wrong file format! An error occurred while reading from the file.\n");
                goto error;
            }
            if (read_line == 1 && !we) { // Address was not read from file
                fprintf(stderr, "Wrong file format!\n");
                goto error;
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

        config->requests[config->numRequests].addr = addr;
        config->requests[config->numRequests].data = data;

        if (we == 'R' || we == 'r') {
            config->requests[config->numRequests].we = 0;
        } else if (we == 'W' || we == 'w') {
            config->requests[config->numRequests].we = 1;
        } else {
            fprintf(stderr, "Error:'%c' is not a valid operation in input file\n", we);
            goto error;
        }

        (config->numRequests)++;

        if (ferror(file)) {
            fprintf(stderr, "Error reading from file. Error code: %d\n", ferror(file));
            goto error;
        }

    } while (!feof(file));

    fclose(file);
    return;

error:
    fclose(file);
    print_usage(progname);
    free(config->requests);
    config->requests = NULL;
    exit(EXIT_FAILURE);
}
