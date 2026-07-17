#include "file.h"
#include "arena.h"
#include "log.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32) && defined(_MSC_VER)
#include <fcntl.h>
#include <io.h>
#include <share.h>

// _sopen_s is the non-deprecated _open (silences C4996): same flags plus an
// explicit share mode; _SH_DENYNO matches POSIX open's no-locking behavior
static int open_file_rdo(const char *path) {
    int fd = -1;
    if (_sopen_s(&fd, path, _O_RDONLY | _O_BINARY, _SH_DENYNO, 0) != 0) {
        return -1;
    }
    return fd;
}

static long read_file(int fd, void *buf, size_t count) { return _read(fd, buf, (unsigned int)count); }

static void close_file(int fd) { _close(fd); }

#else
#include <fcntl.h>
#include <unistd.h>

static int open_file_rdo(const char *path) { return open(path, O_RDONLY); }

static long read_file(int fd, void *buf, size_t count) { return (long)read(fd, buf, count); }

static void close_file(int fd) { close(fd); }

#endif

file_details_t open_file(arena_t *arena, const char *path) {
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

    file_details.contents = arena_alloc(arena, file_size + 1);

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
