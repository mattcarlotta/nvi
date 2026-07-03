#ifndef FILE_H
#define FILE_H

#include <stddef.h>

#define MAX_FILE_SIZE ((size_t)10 * 1024 * 1024)

#if defined(_WIN32) && defined(_MSC_VER)
#include "shims.h"
#include <windows.h>
#define stat_path(path, st) stat((path), (st))
#define PATH_SEP "\\"
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#else
#include <dirent.h>
#include <limits.h>
#define stat_path(path, st) lstat((path), (st))
#define PATH_SEP "/"
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#endif

#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

typedef struct {
    char *contents;
    const char *path;
    size_t len;
} file_details_t;

file_details_t open_file(const char *path);

#endif
