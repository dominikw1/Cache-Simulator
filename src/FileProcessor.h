#pragma once
#include <sys/stat.h>

void validate_file_format(const char* progname, FILE* file, const char* filename, const char* filetype);

FILE* check_file(const char* progname, const char* filename);

void extract_file_data(const char* progname, FILE* file, struct Configuration* config);
