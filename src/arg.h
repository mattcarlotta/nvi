#ifndef ARG_H
#define ARG_H
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
    list_t scan_exts;
    command_t command;
} arg_t;

void print_flags(arg_t *args);
void print_help(void);
void print_version(void);
flag_t get_flag(const char *arg);
result_t set_flag_params(arg_t *args, list_t *list, char *flag);
result_t arg_parser(arg_t *args);
void free_args(arg_t *args);

#endif
