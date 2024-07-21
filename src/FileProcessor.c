#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "ArgParsing.h"
#include "FileProcessor.h"
#include "Request.h"

/**
 * Checks if the file given has the extension 'filetype'
 * Lines 18-22 taken and adapted from https://stackoverflow.com/questions/5309471/getting-file-extension-in-c
 */
int validate_file_format(const char* filename, const char* filetype) {
    const char *dot = strrchr(filename, '.');
    if (dot == NULL || dot == filename) {
        fprintf(stderr, "Error: %s is not a valid file\n", filename);
        return EXIT_FAILURE;
    } else if (strcmp(dot + 1, filetype) != 0) {
        fprintf(stderr, "Error: %s is not a valid %s file!\n", filename, filetype);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * Opens and checks the validity of a file.
 */
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

/**
 * Extracts and validates data from a file and saves it in a 'Request structure'
 * Inspired by: https://github.com/portfoliocourses/c-example-code/blob/main/csv_to_struct_array.c
 */
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


/**
 * Validates trace file name and displays an error message if the file is invalid and exits the program.
 * Lines 190-193 taken and adapted from https://stackoverflow.com/questions/5309471/getting-file-extension-in-c
*/
void check_trace_file(const char* progname, const char* optarg) {
    struct stat file_info;
    if (stat(optarg, &file_info) == 0) {    // Filepath already exists
        if (S_ISDIR(file_info.st_mode)) {
            fprintf(stderr, "Error: Filename '%s' is a directory name. "
                            "Please choose a different filename for the tracefile.\n", optarg);
        } else {
            fprintf(stderr,"Error: File '%s' already exists. "
                            "Please choose a different filename for the tracefile.\n", optarg);
        }
        print_usage(progname);
        exit(EXIT_FAILURE);
    } else {    // Check for invalid filepath
        const char *slash = strrchr(optarg, '/');
        if (slash == NULL || slash == optarg) {
            return;    // No '/' found
        } else if (*(slash + 1) == '\0') {
            fprintf(stderr, "Error: Filepath '%s' does not exist and is a directory name. "
                            "Please choose a different filename for the tracefile.\n", optarg);
        } else {
            // Validate filepath before '/'
            size_t path_length = slash - optarg;
            char path[path_length + 1];
            strncpy(path, optarg, path_length);
            path[path_length] = '\0';

            if (stat(path, &file_info) == 0) {  // Filepath exists
                return;
            }
            fprintf(stderr, "Error: Filepath '%s' does not exist. "
                            "Please choose a different filename for the tracefile.\n", optarg);
        }
        print_usage(progname);
        exit(EXIT_FAILURE);
    }
}
