#ifndef TIMER_H
#define TIMER_H

// Monotonic wall-time in seconds, for measuring elapsed durations. Monotonic
// (not the system clock) so it is unaffected by NTP steps or clock changes
// during the run.

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

#endif
