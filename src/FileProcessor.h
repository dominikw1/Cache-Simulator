#pragma once

FILE* check_file(const char* progname, const char* filename);

int extract_file_data(const char* progname, const char* filename, FILE* file, struct Configuration* config);
