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

int validate_file_format(const char* filename, const char* filetype) {
    // Taken and adapted from https://stackoverflow.com/questions/5309471/getting-file-extension-in-c
    const char *dot = strrchr(filename, '.');   // Check for valid file format
    if (dot == NULL || dot == filename) {
        fprintf(stderr, "Error: %s is not a valid file\n", filename);
        return EXIT_FAILURE;
    } else if (strcmp(dot + 1, filetype) != 0) {
        fprintf(stderr, "Error: %s is not a valid %s file!\n", filename, filetype);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

FILE* check_file(const char* progname, const char* filename) {
    struct stat file_info;
    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "Error opening input file: %s\n", strerror(errno));
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    if (stat(filename, &file_info) != 0) {
        perror("Error determining file size");
        goto error;
    }

    if (S_ISDIR(file_info.st_mode)) {
        fprintf(stderr, "Error: Filename should not be a directory.\n");
        goto error;
    }
    if (file_info.st_size == 0) {
        fprintf(stderr, "Error: Input file contains no data.\n");
        goto error;
    }

    // Check if file is csv file
    int notValid = validate_file_format(filename, "csv");
    if (notValid) {
        goto error;
    }

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
int extract_file_data(const char* progname, const char* filename, FILE* file, struct Configuration* config) {
    struct stat file_info;
    if (stat(filename, &file_info) != 0) {
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
    int character;  // Used to check right file format
    char *comma;
    char line[file_info.st_size];

    do {
        char we;
        int addr;
        int data;

        fgets(line, sizeof(line), file);
        read_line = sscanf(line, "%c,%i,%i\n", &we, &addr, &data);
        character = line[0];
        char* l = line;

        if (read_line != 3) {
            if (strchr(l, ',') == NULL) {
                fprintf(stderr, "Error: Wrong file format! An error occured while reading from the file!\n");
                goto error;
            }
            if (character != 'W' && character != 'R' && character != 'w' && character != 'r') {
                fprintf(stderr, "Error: Wrong file format! First column is not set right!\n");
                goto error;
            }

            comma = strchr(l, ','); // Check if address is given
            if (strncmp(comma + 1, "\0", 1) == 0) {
                fprintf(stderr, "Error: Wrong file format! No address given.\n");
                goto error;
            }
        }

        if ((we == 'W' || we == 'w') && read_line < 3) { // Write should have data written in the file
            fprintf(stderr, "Error: Wrong file format! No data saved.\n");
            goto error;
        }
        if ((we == 'R' || we == 'r') && read_line == 3) { // Read should not have data written in the file
            fprintf(stderr, "Error: Wrong file format! When reading from a file, data should be empty.\n");
            goto error;
        }

        config->requests[config->numRequests].addr = (uint32_t) addr;
        config->requests[config->numRequests].data = (uint32_t) data;

        if (we == 'R' || we == 'r') {
            config->requests[config->numRequests].we = 0;
        } else if (we == 'W' || we == 'w') {
            config->requests[config->numRequests].we = 1;
        } else {
            fprintf(stderr, "Error: '%c' is not a valid operation in input file\n", we);
            goto error;
        }

        (config->numRequests)++;

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
    free(config->requests);
    config->requests = NULL;
    exit(EXIT_FAILURE);
}
