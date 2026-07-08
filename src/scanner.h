#ifndef SCANNER_H
#define SCANNER_H
#include "accessors.h"
#include "arena.h"
#include "arg.h"
#include "set.h"

// The scanner is strictly responsible for recursively walking through the CWD for files
// containing ENV keys and marking them as required. The goal for scanner is to
// allow an engineer to jump into the codebase knowing that ENVs are at least defined
// before a command is ran.

typedef struct {
    size_t dirs_scanned;
    size_t files_scanned;
    size_t references;
    set_t envs;
    const file_ext_map_t *scan_exts;
} scanner_t;

result_t run_scanner(arena_t *arena, args_t *args, scanner_t *scanner);
void merge_required_envs(arena_t *arena, args_t *args, const scanner_t *scanner);

#endif // SCANNER_H
