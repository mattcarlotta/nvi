#ifndef SCANNER_H
#define SCANNER_H
#include "accessors.h"
#include "arg.h"
#include "list.h"
#include <stdbool.h>

typedef struct {
    const char *ext;
    const accessor_t *accessors;
    size_t accessor_count;
} file_ext_t;

typedef struct {
    file_ext_t *items;
    size_t count;
    size_t capacity;
} file_ext_map_t;

typedef struct {
    size_t dirs_scanned;
    size_t files_scanned;
    size_t references;
    bool dry_run;
    list_t envs;
    file_ext_map_t scan_exts;
} scanner_t;

result_t run_scanner(args_t *args, scanner_t *scanner);
void free_scanner(scanner_t *scanner);

#endif
