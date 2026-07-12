#ifndef ARG_H
#define ARG_H

#include "accessors.h"
#include "arena.h"
#include "format.h"
#include "result.h"
#include "set.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Rolling a custom argv parser to support short (-), long (--) and command style flags
//
// Supported flags:
// command -> a command to emit with ENVs to stdout
// dry-run -> displays info to stderr
// files -> a list of .env files to tokenize and parse
// format -> type of format (nul delimited or powershell env delimited) to emit ENVs
// help -> displays help info to stdout
// ignored -> a list of ENV keys that will be ignored (mostly useful for scans)
// required -> a list of ENV keys to mark as required and defined before a command is emitted
// reveal -> exposes ENV values during a dry run
// scan -> a list of file extensions to scan for in the CWD
// threads -> maximum number of threads to use for scanning
// version -> displays current binary info

typedef enum {
    DRY_RUN_FLAG,
    END_OF_OPTIONS,
    FILES_FLAG,
    FORMAT_FLAG,
    HELP_FLAG,
    IGNORED_FLAG,
    REQUIRED_FLAG,
    REVEAL_FLAG,
    SCAN_FLAG,
    THREADS_FLAG,
    UNKNOWN_FLAG,
    VERSION_FLAG
} flag_t;

typedef struct {
    const char **items;
    size_t count;
} command_t;

typedef struct {
    int i;
    int argc;
    const char **argv;
    bool dry_run;
    bool reveal;
    uint8_t scan_threads;
    format_t format;
    set_t files;
    set_t required;
    set_t ignored;
    file_ext_map_t scan_exts;
    command_t command;
} args_t;

result_t parse_args(arena_t *arena, int argc, const char **argv, args_t *args);

#endif // ARG_H
