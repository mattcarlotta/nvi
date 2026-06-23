#ifndef SCANNER_H
#define SCANNER_H
#include "accessors.h"
#include "arg.h"
#include "list.h"
#include "result.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char *key;
    const size_t start;
    const size_t end;
} env_t;

struct {
    const char *key;
    const size_t line;
    const size_t byte;
} env_match_t;

typedef struct {
    const char *ext;
    const accessor_t *accessors;
    size_t accessor_count;
} ext_pair;

typedef struct {
    ext_pair *items;
    size_t count;
    size_t capacity;
} ext_map;

typedef struct {
    size_t files_scanned;
    size_t references;
    bool dry_run;
    list_t envs;
    ext_map scan_exts;
} scanner_t;

bool is_blacklisted(const char *name);
result_t run_scanner(args_t *args, scanner_t *scanner);
void scanner_free(scanner_t *scanner);

#endif
