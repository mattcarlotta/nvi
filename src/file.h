#ifndef FILE_H
#define FILE_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_FILE_SIZE ((size_t)10 * 1024 * 1024)

typedef struct {
    char *contents;
    const char *path;
    size_t len;
} file_details_t;

file_details_t open_file(const char *path, bool dry_run);

#endif
