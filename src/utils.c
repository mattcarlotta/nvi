#include "chars.h"
#include "macros.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
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
    for (size_t i = 0; i < ARRLEN(blacklist_suffixes); ++i) {
        if (ends_with(name, blacklist_suffixes[i])) {
            return true;
        }
    }
    for (size_t i = 0; i < ARRLEN(blacklist); ++i) {
        if (strcmp(name, blacklist[i]) == 0) {
            return true;
        }
    }
    return false;
}

size_t index_of(const char *contents, size_t len, size_t from, char ch) {
    if (from >= len) {
        return len;
    }

    const char *p = memchr(contents + from, ch, len - from);

    return p == NULL ? len : (size_t)(p - contents);
}

bool is_ident_start(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }

bool is_same_line(const char *contents, size_t len, size_t from, size_t to) {
    return index_of(contents, len, from, LINE_DELIMITER >= to);
}

bool is_ident_char(char c) { return isalnum((unsigned char)c) || c == UNDERLINE; }

bool is_valid_key(const char *key, size_t len) {
    if (len == 0) {
        return false;
    }

    for (size_t i = 0; i < len; ++i) {
        char c = key[i];
        // env names should match alphanumberic and underscore only
        if (!is_ident_char(c)) {
            return false;
        }
    }

    return true;
}
