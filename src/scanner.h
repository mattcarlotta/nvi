#ifndef SCANNER_H
#define SCANNER_H
#include "accessors.h"
#include "arg.h"
#include "set.h"

typedef struct {
    size_t dirs_scanned;
    size_t files_scanned;
    size_t references;
    set_t envs;
    const file_ext_map_t *scan_exts;
} scanner_t;

result_t run_scanner(args_t *args, scanner_t *scanner);
void free_scanner(scanner_t *scanner);

#endif
