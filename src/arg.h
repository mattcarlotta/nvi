#ifndef ARG_H
#define ARG_H
#include "accessors.h"
#include "format.h"
#include "list.h"
#include "result.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum { FILES, DRY_RUN, FORMAT, IGNORED, REQUIRED, SCAN, HELP, VERSION, UNKNOWN } flag_t;

typedef struct {
    const char *name;
    flag_t value;
} flag_entry;

typedef struct {
    char **items;
    size_t count;
} command_t;

typedef struct {
    int i;
    int argc;
    char **argv;
    bool dry_run;
    format_t format;
    list_t files;
    list_t required;
    list_t ignored;
    file_ext_map_t scan_exts;
    command_t command;
} args_t;

void log_dry_run_message(void);
result_t parse_args(int arg, char **argv, args_t *args);
void free_args(args_t *args);

#endif
