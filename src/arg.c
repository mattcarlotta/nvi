#include "arg.h"
#include "accessors.h"
#include "chars.h"
#include "errors.h"
#include "format.h"
#include "log.h"
#include "macros.h"
#include "nthread.h"
#include "result.h"
#include "utils.h"
#include "version.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *name;
    flag_t value;
} flag_entry_t;

static void report_flag_items(const char *label, const char **items, size_t count, const char *sep) {
    log_f(SINK_STDERR, "\n    \u2022");
    log_info(SINK_STDERR, " %s: ", label);

    if (count == 0) {
        log_comment(SINK_STDERR, "(empty)");
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        if (i != 0) {
            log_f(SINK_STDERR, "%s", sep);
        }
        log_f(SINK_STDERR, "%s", items[i]);
    }
}

static void report_flag_threads(const uint8_t threads) {
    log_f(SINK_STDERR, "\n    \u2022");
    log_info(SINK_STDERR, " scan threads: ");
    log_f(SINK_STDERR, "%d", threads);
}

static void report_flag_format(const format_t format) {
    log_f(SINK_STDERR, "\n    \u2022");
    log_info(SINK_STDERR, " format: ");
    log_f(SINK_STDERR, "%s\n\n", get_format_name(format));
}

static void report_flag_scan_extensions(const char *label, const file_ext_map_t *map, const char *sep) {
    log_f(SINK_STDERR, "\n    \u2022");
    log_info(SINK_STDERR, " %s: ", label);

    if (map->count == 0) {
        log_comment(SINK_STDERR, "(empty)");
        return;
    }

    for (size_t i = 0; i < map->count; ++i) {
        if (i != 0) {
            log_f(SINK_STDERR, "%s", sep);
        }
        log_f(SINK_STDERR, "%s", map->items[i].ext);
    }
}

static void report_flags(const args_t *args) {
    if (!args->dry_run) {
        return;
    }

    log_info(SINK_STDERR, "\n[INFO]");
    log_f(SINK_STDERR, " The following flags have been set...");
    report_flag_items("command", args->command.items, args->command.count, " ");
    report_flag_items("files", args->files.items, args->files.count, ", ");
    report_flag_items("ignored ENVs", args->ignored.items, args->ignored.count, ", ");
    report_flag_items("required ENVs", args->required.items, args->required.count, ", ");
    report_flag_scan_extensions("scan extensions", &args->scan_exts, ", ");
    report_flag_threads(args->scan_threads);
    report_flag_format(args->format);
}

static void report_command_skipped_warning(const args_t *args) {
    if (!args->dry_run) {
        return;
    }

    log_warning(SINK_STDERR, "\n[WARNING]");
    log_f(SINK_STDERR, " Found a command with dry-run enabled; skipping.\n");
}

static const flag_entry_t flags[] = {
    FLAG("--", END_OF_OPTIONS),
    FLAG("-f", "--files", FILES_FLAG),
    FLAG("-d", "--dry-run", DRY_RUN_FLAG),
    FLAG("-h", "--help", "help", HELP_FLAG),
    FLAG("-i", "--ignored", IGNORED_FLAG),
    FLAG("-F", "--format", FORMAT_FLAG),
    FLAG("-r", "--required", REQUIRED_FLAG),
    FLAG("-s", "--scan", "scan", SCAN_FLAG),
    FLAG("-t", "--threads", THREADS_FLAG),
    FLAG("-v", "--version", "version", VERSION_FLAG),
};

static const size_t flags_len = ARR_LEN(flags);

static inline flag_t get_flag(const char *arg) {
    for (size_t i = 0; i < flags_len; ++i) {
        if (strcmp(arg, flags[i].name) == 0) {
            return flags[i].value;
        }
    }

    return UNKNOWN_FLAG;
}

static const char *get_next_param(args_t *args) {
    if (args->i + 1 >= args->argc) {
        return NULL;
    }

    const char *val = args->argv[args->i + 1];
    if (val[0] == DASH) {
        return NULL;
    }

    ++args->i;

    return val;
}

static result_t get_next_value(args_t *args, const char *flag, const char **param) {
    *param = get_next_param(args);
    if (*param == NULL) {
        return usage_error("The '%s' flag requires at least one argument", flag);
    }

    return RESULT_OK;
}

static bool is_env_file(const char *base) {
    // .env
    if (strcmp(base, ".env") == 0) {
        return true;
    }

    // .env.local
    if (strncmp(base, ".env.", 5) == 0) {
        return true;
    }

    const size_t base_len = strlen(base);

    // example.env
    if (base_len >= 4 && strcmp(base + base_len - 4, ".env") == 0) {
        return true;
    }

    return false;
}

static inline result_t validate_file_name(const char *p) {
    const char *base = path_basename(p);

    if (!is_env_file(base)) {
        return operation_error("The 'files' flag '%s' is an invalid .env file (missing '.env' extension)\n", p);
    }

    if (is_absolute_path(p)) {
        return operation_error("The 'files' flag '%s' must be relative to the current directory\n", p);
    }

    if (path_escapes_cwd(p)) {
        return operation_error("The files flag param '%s' may not escape the current directory\n", p);
    }

    return RESULT_OK;
}

result_t parse_args(arena_t *arena, int argc, const char **argv, args_t *args) {
    // skip program name
    args->i = 1;
    args->argc = argc;
    args->argv = argv;
    args->format = get_default_format();
    args->dry_run = false;
    args->scan_threads = 1;

    result_t result = RESULT_OK;

    while (args->i < args->argc) {
        const char *arg = args->argv[args->i];
        switch (get_flag(arg)) {
            case DRY_RUN_FLAG: {
                args->dry_run = true;
                break;
            }
            case END_OF_OPTIONS: {
                if (args->dry_run) {
                    report_command_skipped_warning(args);
                    args->i = args->argc - 1;
                    break;
                }

                args->command.count = args->argc - (args->i + 1);
                args->command.items = &args->argv[args->i + 1];
                args->i = args->argc - 1;
                break;
            }
            case FILES_FLAG: {
                const char *param;
                result = get_next_value(args, "files", &param);
                if (!result.ok) {
                    return result;
                }

                while (param) {
                    result = validate_file_name(param);
                    if (!result.ok) {
                        return result;
                    }

                    set_add(arena, &args->files, param);
                    param = get_next_param(args);
                }

                break;
            }
            case FORMAT_FLAG: {
                const char *param;
                result = get_next_value(args, "format", &param);
                if (!result.ok) {
                    return result;
                }

                const format_t format = get_format(param);
                if (format == FORMAT_UNKNOWN) {
                    return usage_error(
                        "The 'format' flag contains an invalid ENV format '%s' (expected: nul|powershell)", param);
                }

                args->format = format;
                break;
            }
            case IGNORED_FLAG: {
                const char *param;
                result = get_next_value(args, "ignored", &param);
                if (!result.ok) {
                    return result;
                }

                while (param) {
                    set_add(arena, &args->ignored, param);
                    param = get_next_param(args);
                }

                break;
            }
            case REQUIRED_FLAG: {
                const char *param;
                result = get_next_value(args, "required", &param);
                if (!result.ok) {
                    return result;
                }

                while (param) {
                    set_add(arena, &args->required, param);
                    param = get_next_param(args);
                }

                break;
            }
            case SCAN_FLAG: {
                const char *param;
                result = get_next_value(args, "scan", &param);
                if (!result.ok) {
                    return result;
                }

                while (param) {
                    const file_ext_t *entry = get_scan_extension(param);
                    if (entry == NULL) {
                        return usage_error("The 'scan' flag contains the '%s' file extension that is not supported",
                                           param);
                    }

                    append_file_extension(arena, &args->scan_exts, entry);
                    param = get_next_param(args);
                }

                break;
            }
            case THREADS_FLAG: {
                const char *param;
                result = get_next_value(args, "threads", &param);
                if (!result.ok) {
                    return result;
                }

                const int MAX_CPU_CORES = cpu_count();
                int threads = str_to_u8(param);
                if (threads < 1 || threads > MAX_CPU_CORES) {
                    return usage_error("The 'threads' flag only supports up to %d thread%s, instead found %s",
                                       MAX_CPU_CORES, TO_PLURAL(MAX_CPU_CORES), param);
                }

                args->scan_threads = (uint8_t)threads;
                break;
            }
            case HELP_FLAG: {
                fputs(
                    "Usage: nvi [flags] -- <command>\n"
                    "\n"
                    "Flags:\n"
                    "  -d, --dry-run                prints flags, scan results, file tokens and parsed ENVs to stderr\n"
                    "  -f, --files <paths>          parses .env files in sequential order (at 1 .env file must be "
                    "specified)\n"
                    "  -F, --format <fmt>           formats ENVs for the downsteam consumer (options: nul|powershell)\n"
                    "  -h, --help, help             prints this help and exits with 0\n"
                    "  -i, --ignored <keys>         ignores ENV keys that scan may find and add to the required ENV "
                    "key list\n"
                    "  -r, --required <keys>        ensures ENV keys are defined before the <command> is emitted\n"
                    "  -s, --scan <ext>             recursively scans for ENV variables in <ext> (see options below) "
                    "\u2020\n"
                    "  -t, --threads <1-255>        number of threads to use when scanning for ENV variables (max: "
                    "your CPU core count) \u2020\u2020\n"
                    "  -v, --version, version       prints the version and exits with 0\n"
                    "\n"
                    " \u2020 without a <command>, scan reports what it finds and exits; with a <command>, the found "
                    "ENV keys are added to the required ENV list\n"
                    " \u2020\u2020 using more threads than available CPU cores and/or the OS's IO limitations "
                    "will degrade scanning performance\n"
                    "\n"
                    "Supported scan file extensions (to the right -> of the language):\n"
                    " \u2022 C -> c, h\n"
                    " \u2022 Clojure -> clj, cljs, cljc\n"
                    " \u2022 Crystal -> cr\n"
                    " \u2022 C++ -> cc, cpp, cxx, hh, hpp, hxx\n"
                    " \u2022 C# -> cs\n"
                    " \u2022 D -> d\n"
                    " \u2022 Dart -> dart\n"
                    " \u2022 Elixir -> ex, exs\n"
                    " \u2022 Erlang -> erl, hrl\n"
                    " \u2022 Fortran -> f, f90, f95, f03, f08, for\n"
                    " \u2022 F# -> fs, fsi, fsx\n"
                    " \u2022 Go -> go\n"
                    " \u2022 Gradle -> gradle\n"
                    " \u2022 Groovy -> groovy\n"
                    " \u2022 Haskell -> hs, lhs\n"
                    " \u2022 Java -> java\n"
                    " \u2022 JavaScript/TypeScript -> cjs, cts, js, jsx, mjs, mts, ts, tsx\n"
                    " \u2022 Julia -> jl\n"
                    " \u2022 Kotlin -> kt, kts\n"
                    " \u2022 Lua -> lua\n"
                    " \u2022 Nim -> nim\n"
                    " \u2022 Nushell -> nu\n"
                    " \u2022 Objective-C -> m, mm\n"
                    " \u2022 OCaml -> ml, mli\n"
                    " \u2022 Pascal/Delphi -> dpr, pas, pp\n"
                    " \u2022 Perl -> pl, pm, t\n"
                    " \u2022 PHP -> php\n"
                    " \u2022 PowerShell -> ps1, psm1, psd1\n"
                    " \u2022 Python -> py, pyi, pyw\n"
                    " \u2022 R -> r\n"
                    " \u2022 Ruby -> gemspec, rb, rake\n"
                    " \u2022 Rust -> rs\n"
                    " \u2022 Scala -> sc, scala\n"
                    " \u2022 Swift -> swift\n"
                    " \u2022 Tcl -> tcl\n"
                    " \u2022 V -> v\n"
                    " \u2022 Visual Basic -> vb\n"
                    " \u2022 YAML -> yaml, yml\n"
                    " \u2022 Zig -> zig\n"
                    "\n",
                    stdout);

                return EXIT_EARLY;
            }
            case VERSION_FLAG: {
                fprintf(stdout, "nvi %s (%s)\n", NVI_VERSION, NVI_BUILD);
                fprintf(stdout, "commit %s\n", NVI_COMMIT);
#ifdef __clang__
                fprintf(stdout, "clang %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
#endif
                fprintf(stdout, "%s\n", NVI_TARGET);

                return EXIT_EARLY;
            }
            default: {
                if (arg[0] == DASH) {
                    return usage_error("Unrecognized flag '%s'", arg);
                }

                return usage_error("Unexpected argument '%s'", arg);
            }
        }

        ++args->i;
    }

    report_flags(args);

    if (args->scan_exts.count == 0 && args->files.count == 0) {
        return usage_error("The '--files' or '--scan' flag requires at least one argument");
    }

    if (args->ignored.count > 0 && args->required.count > 0) {
        for (size_t i = 0; i < args->ignored.count; ++i) {
            if (set_contains(&args->required, args->ignored.items[i])) {
                return usage_error("The '%s' key cannot be both required and ignored.", args->ignored.items[i]);
            }
        }
    }

    return result;
}
