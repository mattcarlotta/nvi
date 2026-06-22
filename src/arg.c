#include "arg.h"
#include <string.h>

static const flag_entry flags[] = {
    {"-f", FILES},    {"--files", FILES},

    {"-d", DRY_RUN},  {"--dry-run", DRY_RUN},

    {"-h", HELP},     {"--help", HELP},         {"help", HELP},

    {"-i", IGNORED},  {"--ignored", IGNORED},

    {"-F", FORMAT},   {"--format", FORMAT},

    {"-r", REQUIRED}, {"--required", REQUIRED},

    {"-s", SCAN},     {"--scan", SCAN},         {"scan", SCAN},

    {"-v", VERSION},  {"--version", VERSION},   {"version", VERSION},
};

enum flag_t get_flag(const char *arg) {
    size_t n = sizeof(flags) / sizeof(flags[0]);

    for (size_t i = 0; i < n; ++i) {
        if (strcmp(arg, flags[i].name) == 0) {
            return flags[i].value;
        }
    }

    return UNKNOWN;
}
