#include "arg.h"
#include "list.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const flag_entry flags[] = {
    {.name = "-f", .value = FILES},    {.name = "--files", .value = FILES},

    {.name = "-d", .value = DRY_RUN},  {.name = "--dry-run", .value = DRY_RUN},

    {.name = "-h", .value = HELP},     {.name = "--help", .value = HELP},         {.name = "help", .value = HELP},

    {.name = "-i", .value = IGNORED},  {.name = "--ignored", .value = IGNORED},

    {.name = "-F", .value = FORMAT},   {.name = "--format", .value = FORMAT},

    {.name = "-r", .value = REQUIRED}, {.name = "--required", .value = REQUIRED},

    {.name = "-s", .value = SCAN},     {.name = "--scan", .value = SCAN},         {.name = "scan", .value = SCAN},

    {.name = "-v", .value = VERSION},  {.name = "--version", .value = VERSION},   {.name = "version", .value = VERSION},
};

flag_t get_flag(const char *arg) {
    size_t n = sizeof(flags) / sizeof(flags[0]);

    for (size_t i = 0; i < n; ++i) {
        if (strcmp(arg, flags[i].name) == 0) {
            return flags[i].value;
        }
    }

    return UNKNOWN;
}

void set_flag_params(arg_t *args, list_t *list) {
    while (args->i + 1 < args->argc && args->argv[args->i + 1][0] != '-') {
        ++(args->i);
        list_append(list, args->argv[args->i]);
    }
}

arg_t arg_parser(int argc, char **argv) {
    arg_t args = {.i = 1,
                  .argc = argc,
                  .argv = argv,
                  .command = {.count = 0, .items = NULL},
                  .dry_run = false,
                  .files = {0},
                  .ignored = {0},
                  .required = {0},
                  .scan = {0}};

    while (args.i < args.argc) {
        if (strcmp(args.argv[args.i], "--") == 0) {
            size_t start = args.i + 1;
            args.command.count = args.argc - start;
            args.command.items = &args.argv[start];
            break;
        }

        switch (get_flag(args.argv[args.i])) {
            case FILES: {
                set_flag_params(&args, &args.files);
                break;
            }
            case DRY_RUN: {
                args.dry_run = true;
                break;
            }
            case FORMAT: {

                break;
            }
            case IGNORED: {
                set_flag_params(&args, &args.ignored);
                break;
            }
            case REQUIRED: {
                set_flag_params(&args, &args.required);
                break;
            }
            case SCAN: {

                break;
            }
            case HELP: {

                break;
            }
            case VERSION:
            default: {

                break;
            }
        }
        ++args.i;
    }

    return args;
}
