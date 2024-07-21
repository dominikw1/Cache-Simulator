#pragma once

#include <stdio.h>
#include "ArgParsing.h"

/**
 * This function performs basic checks on the specified file expected as the positional argument
 * to ensure that the file is valid for processing. If the file cannot be opened, is empty or
 * invalid an error message is displayed and the program exits.
 *
 * @return A pointer to the opened file, or exits with an error if the file cannot be opened.
 */
FILE* check_file(const char* progname, const char* filename);

/**
 * This function reads data from the specified file and extracts relevant information to
 * save them in a Request structure containing read and write operations used for the simulation.
 * It assumes that the file format is correct and checks if the extracted data is in
 * the right format.
 *
 * @return 0 if successful, or exits with an error if there is an issue with extracting data.
 */
int extract_file_data(const char* progname, const char* filename, FILE* file, struct Configuration* config);

/**
 * This function validates the trace file name specified by the user to ensure it is accessible.
 * It displays an error message if the file is invalid and exits the program.
 */
void check_trace_file(const char* progname, const char* optarg);
