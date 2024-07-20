#pragma once
#include <sys/stat.h>

int validate_file_format(const char* filename, const char* filetype);

FILE* check_file(const char* progname, const char* filename);

int extract_file_data(const char* progname, FILE* file, struct Configuration* config);
