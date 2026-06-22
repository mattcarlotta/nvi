#ifndef ARG_H
#define ARG_H
#include "list.h"
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
    list_t files;
    list_t required;
    list_t ignored;
    list_t scan;
    command_t command;
} arg_t;

flag_t get_flag(const char *arg);
void set_flag_params(arg_t *args, list_t *list);
arg_t arg_parser(int argc, char **argv);

#endif
