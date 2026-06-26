#include "arg.h"
#include "accessors.h"
#include "chars.h"
#include "dynarr.h"
#include "errors.h"
#include "format.h"
#include "info.h"
#include "list.h"
#include "log.h"
#include <ctype.h>
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

void log_dry_run_message(void) {
    log_info("[INFO]");
    log_f(" Exited early because dry-run was enabled\n\n");
}

static inline flag_t get_flag(const char *arg) {
    size_t n = sizeof(flags) / sizeof(flags[0]);

    for (size_t i = 0; i < n; ++i) {
        if (strcmp(arg, flags[i].name) == 0) {
            return flags[i].value;
        }
    }

    return UNKNOWN;
}

static char *next_value(args_t *args) {
    if (args->i + 1 >= args->argc) {
        return NULL;
    }

    char *nxt = args->argv[args->i + 1];
    if (nxt[0] == DASH) {
        return NULL;
    }

    ++args->i;

    return nxt;
}

static result_t require_value(args_t *args, const char *flag, char **out) {
    *out = next_value(args);
    if (*out == NULL) {
        return usage_error("The '%s' flag requires at least one argument", flag);
    }

    return (result_t){.ok = true};
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

static void log_flags(args_t *args) {
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
    log_f("%s\n\n", format_name(args->format));
}

static inline result_t validate_file_name(const char *f) {
    const char *last_slash = strrchr(f, '/');
    const char *last_backslash = strrchr(f, '\\');
    const char *sep = last_slash > last_backslash ? last_slash : last_backslash;
    const char *base = sep ? sep + 1 : f;

    size_t base_len = strlen(base);
    if (!((strcmp(base, ".env") == 0) || (strncmp(base, ".env.", 5) == 0) ||
          (base_len >= 4 && strcmp(base + base_len - 4, ".env") == 0))) {
        return operation_error("The file flag '%s' is not a valid env file (file must contain '.env' extension)\n", f);
    }

    if ((f[0] == FORWARD_SLASH) || (f[0] == BACK_SLASH) || (isalpha((unsigned char)f[0]) && f[1] == COLON)) {
        return operation_error("The file flag '%s' must be relative to the current directory\n", f);
    }

    const char *p = f;
    while (*p) {
        size_t component_len;
        const char *next_slash = strchr(p, '/');
        const char *next_backslash = strchr(p, '\\');

        const char *next_sep;
        if (!next_slash) {
            next_sep = next_backslash;
        } else if (!next_backslash) {
            next_sep = next_slash;
        } else {
            next_sep = next_slash < next_backslash ? next_slash : next_backslash;
        }

        component_len = next_sep ? (size_t)(next_sep - p) : strlen(p);

        if (component_len == 2 && strncmp(p, "..", 2) == 0) {
            return operation_error("The files flag param '%s' may not escape the current directory\n", f);
        }

        p += component_len;
        if (*p == FORWARD_SLASH || *p == BACK_SLASH) {
            p++;
        }
    }

    return (result_t){.ok = true, .code = 0};
}

result_t parse_args(int argc, char **argv, args_t *args) {
    // skip program name
    args->i = 1;
    args->argc = argc;
    args->argv = argv;
    args->format = get_default_format();
    args->dry_run = false;

    result_t result = {.ok = true, .code = 0};

    while (args->i < args->argc) {

        // end of options delimiter
        if (strcmp(args->argv[args->i], "--") == 0) {
            if (args->dry_run) {
                log_warning("\n[WARNING]");
                log_f(" Found a command with dry-run enabled; skipping.\n", stderr);
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
                char *param;
                result = require_value(args, "files", &param);
                if (!result.ok) {
                    return result;
                }

                while (param != NULL) {
                    result = validate_file_name(param);
                    if (!result.ok) {
                        return result;
                    }

                    DYN_ARR_APPEND(&args->files, param);
                    param = next_value(args);
                }

                break;
            }
            case FORMAT: {
                char *param;
                result = require_value(args, "format", &param);
                if (!result.ok) {
                    return result;
                }

                const format_t format = get_format(param);
                if (format == FORMAT_UNKNOWN) {
                    return usage_error("Invalid format '%s' (expected: nul|powershell)", param);
                }

                args->format = format;
                break;
            }
            case IGNORED: {
                char *param;
                result = require_value(args, "ignored", &param);
                if (!result.ok) {
                    return result;
                }

                while (param != NULL) {
                    DYN_ARR_APPEND(&args->ignored, param);
                    param = next_value(args);
                }

                break;
            }
            case REQUIRED: {
                char *param;
                result = require_value(args, "required", &param);
                if (!result.ok) {
                    return result;
                }

                while (param != NULL) {
                    DYN_ARR_APPEND(&args->required, param);
                    param = next_value(args);
                }

                break;
            }
            case SCAN: {
                char *param;
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
                log_help();

                result.ok = false;
                return result;
            }
            case VERSION: {
                log_version();

                result.ok = false;
                return result;
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
