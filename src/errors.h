#ifndef ERRORS_H
#define ERRORS_H
#include "result.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

result_t operation_error(const char *fmt, ...);

result_t usage_error(const char *fmt, ...);

#endif
