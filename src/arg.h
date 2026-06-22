#ifndef ARG_H
#define ARG_H

enum flag_t { FILES, DRY_RUN, FORMAT, IGNORED, REQUIRED, SCAN, HELP, VERSION, UNKNOWN };

typedef struct {
    const char *name;
    enum flag_t value;
} flag_entry;

enum flag_t get_flag(const char *arg);

#endif
