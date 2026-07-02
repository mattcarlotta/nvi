#include "file.h"

#include "log.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32) && defined(_MSC_VER)
#include "shims.h"
#endif

#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

file_details_t open_file(const char *path) {
    file_details_t file_details = {0};
    file_details.path = path;
    FILE *file = NULL;

    // Gate on a regular file up front. On some platforms fopen happily opens
    // directories (and ftell then reports a garbage size); on Windows fopen
    // fails outright on a directory, which would otherwise surface as a
    // generic "Cannot open" instead of this specific error.
    struct stat st;
    if (stat(path, &st) == 0 && !S_ISREG(st.st_mode)) {
        log_error("[ERROR] Cannot read '%s': not a regular file\n", path);
        goto done;
    }

    file = fopen(path, "rb");
    if (file == NULL) {
        log_error("[ERROR] Cannot open '%s' file: %s\n", path, strerror(errno));
        goto done;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size < 0) {
        log_error("[ERROR] Cannot read '%s' file: %s\n", path, strerror(errno));
        goto done;
    }

    if ((size_t)size > MAX_FILE_SIZE) {
        log_warning("[WARNING] The file '%s' exceeds %zu bytes; skipping.\n", path, MAX_FILE_SIZE);
        goto done;
    }

    file_details.contents = malloc(size + 1);
    if (file_details.contents == NULL) {
        log_error("[ERROR] Failed to allocate %ld bytes for file '%s' (system out of memory?); aborting.\n", size + 1,
                  path);
        fclose(file);
        fflush(stderr);
        abort();
    }

    file_details.len = fread(file_details.contents, 1, size, file);
    file_details.contents[file_details.len] = '\0';
    goto done;

done:
    if (file != NULL) {
        fclose(file);
    }
    return file_details;
}
