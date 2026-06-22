#include "arg.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    arg_t args = arg_parser(argc, argv);

    fprintf(stderr, "[INFO] files: ");
    for (size_t i = 0; i < args.files.count; ++i) {
        fprintf(stderr, "%s", args.files.items[i]);
        if (i + 1 < args.files.count)
            fprintf(stderr, " ");
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "[INFO] command: ");
    for (size_t i = 0; i < args.command.count; ++i) {
        fprintf(stderr, "%s", args.command.items[i]);
        if (i + 1 < args.command.count)
            fprintf(stderr, " ");
    }
    fprintf(stderr, "\n");
    fflush(stderr);

    return 0;
}
