#include "arg.h"
#include "errors.h"
#include "format.h"
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

void print_help(void) {
    fputs("Usage: nvi [flags] -- <command>\n"
          "       nvi scan [ext] [ext] ...etc\n"
          "\n"
          "Flags:\n"
          "  -f, --files <paths>     parses .env files in order (default: .env)\n"
          "  -i, --ignored <keys>    ignores ENV keys that a scan may find and add to a required ENV key list\n"
          "  -r, --required <keys>   marks a list of ENV keys that must be present before the command is emitted\n"
          "  -F, --format <fmt>      outputs ENV format (options: nul|powershell)\n"
          "  -d, --dry-run           print parsed flags, tokens, scan results, and the resolved env listing to stderr\n"
          "  -h, --help              prints this help and exits\n"
          "  -s, --scan <ext>        recursively scans in the root directory for ENV keys used within *.<ext> files "
          "and marks them as required \u2020\n"
          "  -v, --version           prints the version and exits\n"
          "\n"
          "Commands:\n"
          "  help                    prints this help and exits\n"
          "  scan <ext>              recursively scans in the root directory for ENV keys used within *.<ext> files "
          "and marks them as required \u2020\n"
          "  version                 prints the version and exits\n"
          "\n"
          " \u2020 without a command, scan reports what it finds and exits; with a command, the found ENV keys are "
          "added to a required ENV key list\n"
          "\n"
          "Supported scan file extensions:\n"
          " \u2022 C -> c\n"
          " \u2022 Clojure -> clj, cljs, cljc\n"
          " \u2022 Crystal -> cr\n"
          " \u2022 C++ -> cc, cpp, cxx, h, hh, hpp, hxx\n"
          " \u2022 C# -> cs\n"
          " \u2022 D -> d\n"
          " \u2022 Dart -> dart\n"
          " \u2022 Elixir -> ex, exs\n"
          " \u2022 Erlang -> erl, hrl\n"
          " \u2022 Fortran -> f, f90, f95, f03, f08, for\n"
          " \u2022 F# -> fs, fsi, fsx\n"
          " \u2022 Go -> go\n"
          " \u2022 Groovy -> groovy\n"
          " \u2022 Haskell -> hs, lhs\n"
          " \u2022 Java -> java, gradle\n"
          " \u2022 JavaScript/TypeScript -> cjs, cts, js, jsx, mjs, mts, ts, tsx\n"
          " \u2022 Julia -> jl\n"
          " \u2022 Kotlin -> kt, kts\n"
          " \u2022 Lua -> lua\n"
          " \u2022 Nim -> nim\n"
          " \u2022 Nushell -> nu\n"
          " \u2022 Objective-C -> m, mm\n"
          " \u2022 OCaml -> ml, mli\n"
          " \u2022 Pascal/Delphi -> ldpr, pas, pp\n"
          " \u2022 Perl -> pl, pm, t\n"
          " \u2022 PHP -> php\n"
          " \u2022 PowerShell -> ps1, psm1, psd1\n"
          " \u2022 Python -> py, pyi\n"
          " \u2022 R -> r\n"
          " \u2022 Ruby -> gemspec, rb, rake\n"
          " \u2022 Rust -> rs\n"
          " \u2022 Scala -> sc, scala\n"
          " \u2022 Swift -> swift\n"
          " \u2022 Tcl -> tcl\n"
          " \u2022 V -> v\n"
          " \u2022 Visual Basic -> vb\n"
          " \u2022 Zig -> zig\n"
          "\n",
          stdout);
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
    result_t res = {.ok = true, .errcode = 0};

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
                set_flag_params(args, &args->files);
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
                set_flag_params(args, &args->ignored);
                break;
            }
            case REQUIRED: {
                set_flag_params(args, &args->required);
                break;
            }
            case SCAN: {
                set_flag_params(args, &args->scan_exts);
                break;
            }
            case HELP: {
                print_help();
                res.ok = false;
                return res;
            }
            case VERSION:
            default: {
                res.ok = false;
                return res;
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

    return res;
}

void free_args(arg_t *args) {
    free_list(&args->files);
    free_list(&args->ignored);
    free_list(&args->required);
    free_list(&args->scan_exts);
}
