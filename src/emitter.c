#include "arg.h"
#include "chars.h"
#include "errors.h"
#include "format.h"
#include "parser.h"
#include "result.h"
#include <stdio.h>

static inline void write_to_ps_format(const char *s) {
    size_t value_len = strlen(s);
    size_t start = 0;

    for (size_t i = 0; i < value_len; ++i) {
        if (s[i] == SINGLE_QUOTE) {
            fwrite(s + start, 1, i - start + 1, stdout);
            fputc('\'', stdout);
            start = i + 1;
        }
    }

    if (start < value_len) {
        fwrite(s + start, 1, value_len - start, stdout);
    }
}

result_t run_emitter(const args_t *args, const env_map_t *env_map) {
    result_t result = {.ok = true, .code = 0};

    switch (args->format) {
        case FORMAT_NULL: {
            for (size_t i = 0; i < env_map->count; ++i) {
                fprintf(stdout, "%s=%s", env_map->items[i].key, env_map->items[i].value);
                fputc('\0', stdout);
            }

            return result;
        }
        case FORMAT_POWERSHELL: {
            for (size_t i = 0; i < env_map->count; ++i) {
                fprintf(stdout, "$env:%s = '", env_map->items[i].key);
                write_to_ps_format(env_map->items[i].value);
                fputc('\'', stdout);
                fprintf(stdout, "\n");
            }

            if (args->command.count > 0) {
                for (size_t i = 0; i < args->command.count; ++i) {
                    fputs(" '", stdout);
                    write_to_ps_format(args->command.items[i]);
                    fputc('\'', stdout);
                }
                fprintf(stdout, "\n");
            }

            return result;
        }
        default: {
            return operation_error("Unknown format %s", format_name(args->format));
        }
    }
}
