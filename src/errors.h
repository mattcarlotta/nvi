#ifndef ERRORS_H
#define ERRORS_H
#include "result.h"

result_t operation_error(const char *fmt, ...);
result_t usage_error(const char *fmt, ...);

#endif // ERRORS_H
