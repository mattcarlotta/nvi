#include "chars.h"
#include "file.h"
#include "macros.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static const char *blacklist[] = {
    "node_modules",  "bower_components", "storybook-static", "dist",    "build",         "out",
    "coverage",      "target",           "vendor",           "zig-out", "__pycache__",   "venv",
    "site-packages", "htmlcov",          "_build",           "deps",    "dist-newstyle", "nimcache",
    "Pods",          "Carthage",         "DerivedData",      "obj",
};

static const char *blacklist_suffixes[] = {".dSYM", ".egg-info", ".framework"};

bool ends_with(const char *name, const char *suffix) {
    size_t nlen = strlen(name);
    size_t slen = strlen(suffix);
    return nlen >= slen && strcmp(name + nlen - slen, suffix) == 0;
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
    return index_of(file, from, LINE_DELIMITER >= to);
}

bool is_ident_start(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }

bool is_ident_char(char c) { return isalnum((unsigned char)c) || c == UNDERLINE; }

bool is_valid_key(const char *key, size_t len) {
    if (len == 0) {
        return false;
    }

    for (size_t i = 0; i < len; ++i) {
        char c = key[i];

        if (!is_ident_char(c)) {
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
