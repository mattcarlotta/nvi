#include "arg.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    printf("Arg count %d \n", argc);
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "--") == 0) {
            printf("End of options delimiter\n");
            break;
        }

        switch (get_flag(argv[i])) {
        case FILES: {
            printf("Flag: %s\n", argv[i]);
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                ++i;
                printf("Params: %s\n", argv[i]);
            }
        }
        case DRY_RUN: {

            break;
        }
        case FORMAT: {

            break;
        }
        case IGNORED: {

            break;
        }
        case REQUIRED: {

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
        ++i;
    }
    return 0;
}
