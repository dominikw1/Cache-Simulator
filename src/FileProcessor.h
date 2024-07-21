#pragma once

#include <stdio.h>
#include "Argparsing.h"

FILE* check_file(const char* progname, const char* filename);

int extract_file_data(const char* progname, const char* filename, FILE* file, struct Configuration* config);

void check_trace_file(const char* progname, const char* optarg);
