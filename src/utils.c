#include "chars.h"
#include "file.h"
#include "macros.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *blacklist[] = {
    "node_modules",  "bower_components", "storybook-static", "dist",    "build",         "out",
    "coverage",      "target",           "vendor",           "zig-out", "__pycache__",   "venv",
    "site-packages", "htmlcov",          "_build",           "deps",    "dist-newstyle", "nimcache",
    "Pods",          "Carthage",         "DerivedData",      "obj",
};

static const char *blacklist_suffixes[] = {".dSYM", ".egg-info", ".framework"};

bool is_path_sep(const char c) { return c == FORWARD_SLASH || c == BACK_SLASH; }

bool is_absolute_path(const char *p) { return is_path_sep(p[0]) || (isalpha((unsigned char)p[0]) && p[1] == COLON); }

const char *path_basename(const char *fp) {
    const char *base = fp;
    for (const char *path = fp; *path; ++path) {
        if (is_path_sep(*path)) {
            base = path + 1;
        }
    }

    return base;
}

bool path_escapes_cwd(const char *path) {
    while (*path) {
        const char *start = path;

        while (*path && !is_path_sep(*path)) {
            // skip character
            ++path;
        }

        if (path - start == 2 && start[0] == DOT && start[1] == DOT) {
            return true;
        }

        // skip the separator
        if (*path) {
            ++path;
        }
    }

    return false;
}

bool ends_with(const char *name, const char *suffix) {
    size_t nlen = strlen(name);
    size_t slen = strlen(suffix);
    return nlen >= slen && memcmp(name + nlen - slen, suffix, slen) == 0;
}

bool is_blacklisted(const char *name) {
    for (size_t i = 0; i < ARR_LEN(blacklist_suffixes); ++i) {
        if (ends_with(name, blacklist_suffixes[i])) {
            return true;
        }
    }
    for (size_t i = 0; i < ARR_LEN(blacklist); ++i) {
        if (strcmp(name, blacklist[i]) == 0) {
            return true;
        }
    }
    return false;
}

size_t index_of_scalar(const char *s, size_t len, size_t pos, int ch) {
    for (size_t k = pos; k < len; ++k) {
        if ((unsigned char)s[k] == (unsigned char)ch) {
            return k;
        }
    }
    return len;
}

size_t index_of(const file_details_t *file, size_t from, char ch) {
    if (from >= file->len) {
        return file->len;
    }

    const char *p = memchr(file->contents + from, ch, file->len - from);

    return p == NULL ? file->len : (size_t)(p - file->contents);
}

bool is_same_line(const file_details_t *file, size_t from, size_t to) {
    return index_of(file, from, LINE_DELIMITER) >= to;
}

bool is_ident_start(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }

bool is_ident_char(char c) { return isalnum((unsigned char)c) || c == UNDERLINE; }

bool is_valid_key(const char *key, size_t len) {
    if (len == 0 || !is_ident_start(key[0])) {
        return false;
    }

    for (size_t i = 1; i < len; ++i) {
        if (!is_ident_char(key[i])) {
            return false;
        }
    }

    return true;
}

void fput_repeat(FILE *f, char c, size_t n) {
    while (n--) {
        fputc(c, f);
    }
}

int str_to_u8(const char *s) {
    const char *p = s;

    while (*p == SPACE || (*p >= TAB && *p <= CARRIAGE_RETURN)) {
        ++p;
    }

    if (*p < '0' || *p > '9') {
        return -1;
    }

    if (*p == '0' && *(p + 1) != '\0') {
        return -1;
    }

    unsigned int v = 0;
    while (*p >= '0' && *p <= '9') {
        v = v * 10 + (unsigned int)(*p - '0');
        if (v > 255) {
            return -1;
        }
        ++p;
    }

    return (*p == '\0') ? (int)v : -1;
}
