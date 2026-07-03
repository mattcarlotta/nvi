#include "arg.h"
#include "accessors.h"
#include "chars.h"
#include "dynarr.h"
#include "errors.h"
#include "format.h"
#include "info.h"
#include "list.h"
#include "log.h"
#include "macros.h"
#include "result.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const flag_entry flags[] = {
    FLAG("-f", "--files", FILES),       FLAG("-d", "--dry-run", DRY_RUN),
    FLAG("-h", "--help", "help", HELP), FLAG("-i", "--ignored", IGNORED),
    FLAG("-F", "--format", FORMAT),     FLAG("-r", "--required", REQUIRED),
    FLAG("-s", "--scan", "scan", SCAN), FLAG("-v", "--version", "version", VERSION),
};

static inline flag_t get_flag(const char *arg) {
    size_t n = sizeof(flags) / sizeof(flags[0]);

    for (size_t i = 0; i < n; ++i) {
        if (strcmp(arg, flags[i].name) == 0) {
            return flags[i].value;
        }
    }

    return UNKNOWN;
}

static const char *next_value(args_t *args) {
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

static result_t require_value(args_t *args, const char *flag, const char **param) {
    *param = next_value(args);
    if (*param == NULL) {
        return usage_error("The '%s' flag requires at least one argument", flag);
    }

    return RESULT_OK;
}

static void log_items(const char *label, const list_t *list, const char *sep) {
    log_f("\n    \u2022");
    log_info(" %s: ", label);

    if (list->count == 0) {
        log_comment("(empty)");
        return;
    }

    for (size_t i = 0; i < list->count; ++i) {
        if (i != 0) {
            log_f("%s", sep);
        }
        log_f("%s", list->items[i]);
    }
}

static void log_ext_items(const char *label, const file_ext_map_t *map, const char *sep) {
    log_f("\n    \u2022");
    log_info(" %s: ", label);

    if (map->count == 0) {
        log_comment("(empty)");
        return;
    }

    for (size_t i = 0; i < map->count; ++i) {
        if (i != 0) {
            log_f("%s", sep);
        }
        log_f("%s", map->items[i].ext);
    }
}

static void log_flags(const args_t *args) {
    log_info("\n[INFO]");
    log_f(" The following flags have been set...");
    log_items("command", (const list_t *)&args->command, " ");
    log_f("\n    \u2022");
    log_info(" dry run: ");
    log_f("on");
    log_items("files", &args->files, ", ");
    log_items("ignored ENVs", &args->ignored, ", ");
    log_items("required ENVs", &args->required, ", ");
    log_ext_items("scan extensions", &args->scan_exts, ", ");
    log_f("\n    \u2022");
    log_info(" format: ");
    log_f("%s\n\n", get_format_name(args->format));
}

static void append_unique_param(list_t *list, const char *value) {
    if (!list_contains(list, value)) {
        DYN_ARR_APPEND(list, value);
    }
}

static inline bool is_env_file(const char *base, size_t base_len) {
    // .env
    if (strcmp(base, ".env") == 0) {
        return true;
    }

    // .env.local
    if (strncmp(base, ".env.", 5) == 0) {
        return true;
    }

    // example.env
    if (base_len >= 4 && strcmp(base + base_len - 4, ".env") == 0) {
        return true;
    }

    return false;
}

static inline result_t validate_file_name(const char *p) {
    const char *base = path_basename(p);

    if (!is_env_file(base, strlen(base))) {
        return operation_error("The file flag '%s' is an invalid .env file (missing '.env' extension)\n", p);
    }

    if (is_absolute_path(p)) {
        return operation_error("The file flag '%s' must be relative to the current directory\n", p);
    }

    if (path_escapes_cwd(p)) {
        return operation_error("The files flag param '%s' may not escape the current directory\n", p);
    }

    return RESULT_OK;
}

result_t parse_args(int argc, const char **argv, args_t *args) {
    // skip program name
    args->i = 1;
    args->argc = argc;
    args->argv = argv;
    args->format = get_default_format();
    args->dry_run = false;

    result_t result = RESULT_OK;

    while (args->i < args->argc) {

        // end of options delimiter
        if (strcmp(args->argv[args->i], "--") == 0) {
            if (args->dry_run) {
                log_warning("\n[WARNING]");
                log_f(" Found a command with dry-run enabled; skipping.\n");
                break;
            }

            args->command.count = args->argc - (args->i + 1);
            args->command.items = &args->argv[args->i + 1];
            break;
        }

        switch (get_flag(args->argv[args->i])) {
            case DRY_RUN: {
                args->dry_run = true;
                break;
            }
            case FILES: {
                const char *param;
                result = require_value(args, "files", &param);
                if (!result.ok) {
                    return result;
                }

                while (param != NULL) {
                    result = validate_file_name(param);
                    if (!result.ok) {
                        return result;
                    }

                    append_unique_param(&args->files, param);
                    param = next_value(args);
                }

                break;
            }
            case FORMAT: {
                const char *param;
                result = require_value(args, "format", &param);
                if (!result.ok) {
                    return result;
                }

                const format_t format = get_format(param);
                if (format == FORMAT_UNKNOWN) {
                    return usage_error("Invalid ENV format '%s' (expected: nul|powershell)", param);
                }

                args->format = format;
                break;
            }
            case IGNORED: {
                const char *param;
                result = require_value(args, "ignored", &param);
                if (!result.ok) {
                    return result;
                }

                while (param != NULL) {
                    append_unique_param(&args->ignored, param);
                    param = next_value(args);
                }

                break;
            }
            case REQUIRED: {
                const char *param;
                result = require_value(args, "required", &param);
                if (!result.ok) {
                    return result;
                }

                while (param != NULL) {
                    append_unique_param(&args->required, param);
                    param = next_value(args);
                }

                break;
            }
            case SCAN: {
                const char *param;
                result = require_value(args, "scan", &param);
                if (!result.ok) {
                    return result;
                }

                while (param != NULL) {
                    const ext_entry *entry = find_ext(param);
                    if (entry == NULL) {
                        return usage_error("The file extension '%s' is not a supported scan extension", param);
                    }

                    file_ext_append(&args->scan_exts, entry);
                    param = next_value(args);
                }

                break;
            }
            case HELP: {
                return log_help();
            }
            case VERSION: {
                return log_version();
            }
            default: {
                const char *token = args->argv[args->i];

                if (token[0] == DASH) {
                    return usage_error("Unrecognized flag '%s'", token);
                }

                return usage_error("Unexpected argument '%s'", token);
            }
        }

        ++args->i;
    }

    if (args->dry_run) {
        log_flags(args);
    }

    if (args->scan_exts.count == 0 && args->files.count == 0) {
        return usage_error("The '--files' or '--scan' flag requires at least one argument");
    }

    return result;
}

void free_args(args_t *args) {
    free_list(&args->files);
    free_list(&args->ignored);
    free_list(&args->required);
    free_file_ext_map(&args->scan_exts);
}
