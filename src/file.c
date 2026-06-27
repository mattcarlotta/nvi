#include "file.h"
#include "log.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

file_details_t open_file(const char *path, bool dry_run) {
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

    if (size <= 0) {
        if (dry_run) {
            log_warning("[WARNING] The file '%s' appears to be empty.\n\n", path);
        }
        goto done;
    }

    if ((size_t)size > MAX_FILE_SIZE) {
        if (dry_run) {
            log_warning("[WARNING] The file '%s' exceeds %zu bytes, skipping.\n\n", path, MAX_FILE_SIZE);
        }
        goto done;
    }

    file_details.contents = malloc(size + 1);
    if (file_details.contents == NULL) {
        log_error("[ERROR] Failed to load file '%s' (file may be empty or not found); aborting.\n\n", path);
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
