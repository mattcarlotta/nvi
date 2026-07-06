#include "file.h"
#include "log.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32) && defined(_MSC_VER)
#include <fcntl.h>
#include <io.h>

static int open_file_rdo(const char *path) { return _open(path, _O_RDONLY | _O_BINARY); }

static long read_file(int fd, void *buf, size_t count) { return _read(fd, buf, (unsigned int)count); }

static void close_file(int fd) { _close(fd); }

#else
#include <fcntl.h>
#include <unistd.h>

static int open_file_rdo(const char *path) { return open(path, O_RDONLY); }

static long read_file(int fd, void *buf, size_t count) { return (long)read(fd, buf, count); }

static void close_file(int fd) { close(fd); }

#endif

file_details_t open_file(const char *path) {
    file_details_t file_details = {0};
    file_details.path = path;

    int fd = open_file_rdo(path);
    if (fd < 0) {
        log_error(SINK_STDERR, "[ERROR] Unable to open '%s' (not a valid file?)\n", path);
        return file_details;
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
        log_error(SINK_STDERR, "[ERROR] Cannot read '%s' file: %s\n", path, strerror(errno));
        goto done;
    }

    if (!S_ISREG(st.st_mode)) {
        log_error(SINK_STDERR, "[ERROR] Unable to open '%s' (not a valid file?)", path);
        goto done;
    }

    size_t file_size = (size_t)st.st_size;

    if (file_size > MAX_FILE_SIZE) {
        log_warning(SINK_STDERR, "[WARNING] The file '%s' exceeds %zu bytes; skipping.\n", path, MAX_FILE_SIZE);
        goto done;
    }

    file_details.contents = malloc(file_size + 1);
    if (file_details.contents == NULL) {
        log_error(SINK_STDERR,
                  "[ERROR] Failed to allocate %zu bytes for file '%s' (system out of memory?); aborting.\n",
                  file_size + 1, path);
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    size_t total = 0;
    while (total < file_size) {
        long n = read_file(fd, file_details.contents + total, file_size - total);

        if (n < 0) {
#if !defined(_WIN32)
            if (errno == EINTR) {
                continue;
            }
#endif
            log_error(SINK_STDERR, "[ERROR] Cannot read '%s' file: %s\n", path, strerror(errno));
            free(file_details.contents);
            file_details.contents = NULL;
            goto done;
        }

        if (n == 0) {
            break;
        }

        total += (size_t)n;
    }

    file_details.len = total;
    if (file_details.contents != NULL) {
        file_details.contents[total] = '\0';
    }

done:
    close_file(fd);
    return file_details;
}
