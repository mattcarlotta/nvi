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

file_details_t open_file(const char *path) {
    file_details_t file_details = {0};
    file_details.path = path;
    FILE *file = fopen(path, "rb");
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

    // an empty file returns a valid zero-length buffer (contents != NULL,
    // len == 0); callers decide whether empty is a warning (scanner) or an
    // error (--files)
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
