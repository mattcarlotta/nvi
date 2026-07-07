#ifndef TIMER_H
#define TIMER_H

// Monotonic wall-time in seconds, for measuring elapsed durations. Monotonic
// (not the system clock) so it is unaffected by NTP steps or clock changes
// during the run.

#include "tty.h"
#include <stdio.h>
#if defined(_WIN32) && defined(_MSC_VER)
#include <windows.h>

static inline double monotonic_seconds(void) {
    LARGE_INTEGER freq, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / (double)freq.QuadPart;
}
#else
#include <time.h>

static inline double monotonic_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}
#endif

static inline void log_dry_run_time(double start) {
    double elapsed_ms = (monotonic_seconds() - start) * 1000.0;
    if (use_color) {
        fprintf(stderr, CYAN);
    }
    fputs("[INFO]", stderr);
    if (use_color) {
        fprintf(stderr, RESET);
    }

    fputs(" Dry run completed in ", stderr);

    if (elapsed_ms >= 1000.0) {
        fprintf(stderr, "%.3fs", elapsed_ms / 1000.0);
    } else {
        fprintf(stderr, "%.1fms", elapsed_ms);
    }

    fputc('\n', stderr);
}
#endif // TIMER_H
