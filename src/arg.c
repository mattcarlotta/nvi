#include "arg.h"
#include "errors.h"
#include "format.h"
#include "info.h"
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

result_t set_flag_params(arg_t *args, list_t *list, char *flag) {
    size_t current_count = list->count;

    while (args->i + 1 < args->argc && args->argv[args->i + 1][0] != '-') {
        ++(args->i);
        list_append(list, args->argv[args->i]);
    }

    if (list->count == current_count) {
        return usage_error("option '%s' requires at least one argument", flag);
    }

    return (result_t){.ok = true, .errcode = 2};
}

void print_flags(arg_t *args) {
    fprintf(stderr, "info: The following flags have been set... \n");
    fprintf(stderr, "    \u2022 command: ");
    if (args->command.count > 0) {
        for (size_t i = 0; i < args->command.count; ++i) {
            if (i != 0)
                fprintf(stderr, " ");
            fprintf(stderr, "%s", args->command.items[i]);
        }
    } else {
        fprintf(stderr, "(undefined)");
    }

    fprintf(stderr, "\n    \u2022 files: ");
    for (size_t i = 0; i < args->files.count; ++i) {
        if (i != 0)
            fprintf(stderr, ", ");
        fprintf(stderr, "%s", args->files.items[i]);
    }

    fprintf(stderr, "\n    \u2022 required ENVs: ");
    if (args->required.count > 0) {
        for (size_t i = 0; i < args->required.count; ++i) {
            if (i != 0)
                fprintf(stderr, ", ");
            fprintf(stderr, "%s", args->required.items[i]);
        }
    } else {
        fprintf(stderr, "(undefined)");
    }

    fprintf(stderr, "\n    \u2022 ignored ENVs: ");
    if (args->ignored.count > 0) {
        for (size_t i = 0; i < args->ignored.count; ++i) {
            if (i != 0)
                fprintf(stderr, ", ");
            fprintf(stderr, "%s", args->ignored.items[i]);
        }
    } else {
        fprintf(stderr, "(undefined)");
    }

    fprintf(stderr, "\n    \u2022 scan extensions: ");
    if (args->scan_exts.count > 0) {
        for (size_t i = 0; i < args->scan_exts.count; ++i) {
            if (i != 0)
                fprintf(stderr, ", ");
            fprintf(stderr, "%s", args->scan_exts.items[i]);
        }
    } else {
        fprintf(stderr, "(undefined)");
    }

    fprintf(stderr, "\n    \u2022 format: %s\n", format_name(args->format));
}

result_t arg_parser(arg_t *args) {
    // skip program name
    args->i = 1;

    while (args->i < args->argc) {

        // end of options delimiter
        if (strcmp(args->argv[args->i], "--") == 0) {
            args->command.count = args->argc - (args->i + 1);
            args->command.items = &args->argv[args->i + 1];
            break;
        }

        switch (get_flag(args->argv[args->i])) {
            case FILES: {
                result_t r = set_flag_params(args, &args->files, "files");
                if (!r.ok)
                    return r;
                break;
            }
            case DRY_RUN: {
                args->dry_run = true;
                break;
            }
            case FORMAT: {
                ++args->i;

                if (args->i >= args->argc) {
                    return usage_error("option '--format' requires an argument", NULL);
                }

                const format_t format = get_format(args->argv[args->i]);
                if (format == FORMAT_UNKNOWN) {
                    return usage_error("invalid format '%s' (expected: nul|powershell)", args->argv[args->i]);
                }

                args->format = format;
                break;
            }
            case IGNORED: {
                result_t r = set_flag_params(args, &args->ignored, "ignored");
                if (!r.ok)
                    return r;
                break;
            }
            case REQUIRED: {
                result_t r = set_flag_params(args, &args->required, "required");
                if (!r.ok)
                    return r;
                break;
            }
            case SCAN: {
                result_t r = set_flag_params(args, &args->scan_exts, "scan");
                if (!r.ok)
                    return r;
                break;
            }
            case HELP: {
                print_help();
                return (result_t){.ok = false, .errcode = 0};
            }
            case VERSION: {
                print_version();
                return (result_t){.ok = false, .errcode = 0};
            }
            default: {
                const char *token = args->argv[args->i];

                if (token[0] == '-') {
                    return usage_error("unrecognized flag '%s'", token);
                }

                return usage_error("unexpected argument '%s'", token);
            }
        }
        ++args->i;
    }

    if (args->files.count == 0) {
        list_append(&args->files, ".env");
    }

    if (args->dry_run) {
        print_flags(args);
    }

    return (result_t){.ok = true};
}

void free_args(arg_t *args) {
    free_list(&args->files);
    free_list(&args->ignored);
    free_list(&args->required);
    free_list(&args->scan_exts);
}
