#ifndef FUZZ_WATCHDOG_H
#define FUZZ_WATCHDOG_H

// Shared progress visibility for libFuzzer harnesses (POSIX only).
//
// 1. A watchdog thread prints a heartbeat every FUZZ_HEARTBEAT_SECONDS (default 5)
//    with total execs, execs/sec since the last beat, and the current input's
//    runtime. It writes to a dup of the real stderr grabbed in a constructor,
//    so it stays visible even when stderr is redirected.
// 2. If a single input runs longer than FUZZ_STALL_SECONDS (default 10), the
//    watchdog dumps it to fuzz-stall-<pid>.bin and aborts, so a genuine hang
//    becomes a reproducible artifact instead of a silent stall.
// 3. fuzz_watchdog_init also redirects the stderr global to /dev/null so the
//    project's per-input [ERROR] output doesn't drown libFuzzer's status
//    lines; the project reads the global per call while libFuzzer captured
//    the original FILE at static init and ASan writes crash reports to fd 2
//    directly, so both stay visible. FUZZ_VERBOSE keeps the output for
//    debugging a single reproducer.
//
// Usage, from a harness:
//   int LLVMFuzzerInitialize(int *argc, char ***argv) {
//       (void)argc; (void)argv;
//       fuzz_watchdog_init("harness name");
//       return 0;
//   }
//   int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
//       fuzz_watchdog_input_begin(data, size);
//       ... exercise the target ...
//       fuzz_watchdog_input_end();
//       return 0;
//   }

#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int fuzz_heartbeat_fd = -1;
static _Atomic uint64_t fuzz_total_execs = 0;
static _Atomic uint64_t fuzz_input_started_ns = 0; // 0 when no input is in flight
static _Atomic size_t fuzz_input_len = 0;
static const uint8_t *volatile fuzz_input_data = NULL;

static uint64_t fuzz_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

// grab the real stderr before libFuzzer or the harness can close/redirect fd 2
__attribute__((constructor)) static void fuzz_grab_heartbeat_fd(void) { fuzz_heartbeat_fd = dup(STDERR_FILENO); }

static void fuzz_heartbeat_printf(const char *fmt, ...) {
    if (fuzz_heartbeat_fd < 0) {
        return;
    }

    char line[256];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    if (n > 0) {
        ssize_t unused = write(fuzz_heartbeat_fd, line, (size_t)n < sizeof(line) ? (size_t)n : sizeof(line) - 1);
        (void)unused;
    }
}

static unsigned fuzz_env_seconds(const char *name, unsigned fallback) {
    const char *v = getenv(name);
    if (v == NULL || v[0] == '\0') {
        return fallback;
    }

    char *end = NULL;
    unsigned long n = strtoul(v, &end, 10);
    return (end != NULL && *end == '\0' && n > 0 && n < 3600) ? (unsigned)n : fallback;
}

static void fuzz_dump_stall_input(void) {
    const uint8_t *data = fuzz_input_data;
    size_t len = atomic_load(&fuzz_input_len);
    if (data == NULL) {
        return;
    }

    char path[64];
    snprintf(path, sizeof(path), "fuzz-stall-%d.bin", (int)getpid());

    FILE *f = fopen(path, "wb");
    if (f != NULL) {
        fwrite(data, 1, len, f);
        fclose(f);
        fuzz_heartbeat_printf("[fuzz] stalled input written to %s (%zu bytes)\n", path, len);
    }
}

static void *fuzz_watchdog_main(void *unused) {
    (void)unused;

    const unsigned beat_s = fuzz_env_seconds("FUZZ_HEARTBEAT_SECONDS", 5);
    const unsigned stall_s = fuzz_env_seconds("FUZZ_STALL_SECONDS", 10);
    const uint64_t started = fuzz_now_ns();
    uint64_t prev_execs = 0;

    for (;;) {
        struct timespec ts = {.tv_sec = beat_s};
        nanosleep(&ts, NULL);

        uint64_t execs = atomic_load(&fuzz_total_execs);
        uint64_t in_flight = atomic_load(&fuzz_input_started_ns);
        uint64_t now = fuzz_now_ns();

        double elapsed_s = (double)(now - started) / 1e9;
        double rate = (double)(execs - prev_execs) / (double)beat_s;
        prev_execs = execs;

        if (in_flight != 0) {
            double input_s = (double)(now - in_flight) / 1e9;

            if (input_s >= (double)stall_s) {
                fuzz_heartbeat_printf("[fuzz] STALL: current input (%zu bytes) has run %.1fs (limit %us); aborting\n",
                                      atomic_load(&fuzz_input_len), input_s, stall_s);
                fuzz_dump_stall_input();
                abort();
            }

            if (input_s >= 0.5) {
                fuzz_heartbeat_printf(
                    "[fuzz] alive: execs=%llu (%.0f/s) elapsed=%.0fs SLOW input: %zu bytes running %.1fs\n",
                    (unsigned long long)execs, rate, elapsed_s, atomic_load(&fuzz_input_len), input_s);
                continue;
            }
        }

        fuzz_heartbeat_printf("[fuzz] alive: execs=%llu (%.0f/s) elapsed=%.0fs\n", (unsigned long long)execs, rate,
                              elapsed_s);
    }

    return NULL;
}

static void fuzz_watchdog_init(const char *harness_name) {
    fuzz_heartbeat_printf("[fuzz] %s harness up; heartbeat every %us, stall limit %us\n", harness_name,
                          fuzz_env_seconds("FUZZ_HEARTBEAT_SECONDS", 5), fuzz_env_seconds("FUZZ_STALL_SECONDS", 10));

    if (getenv("FUZZ_VERBOSE") == NULL) {
        FILE *devnull = fopen("/dev/null", "w");
        if (devnull != NULL) {
            stderr = devnull;
            // help/version output; libFuzzer never writes to stdout
            stdout = devnull;
        }
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, fuzz_watchdog_main, NULL) == 0) {
        pthread_detach(tid);
    }
}

static void fuzz_watchdog_input_begin(const uint8_t *data, size_t size) {
    fuzz_input_data = data;
    atomic_store(&fuzz_input_len, size);
    atomic_store(&fuzz_input_started_ns, fuzz_now_ns());
}

static void fuzz_watchdog_input_end(void) {
    atomic_store(&fuzz_input_started_ns, 0);
    fuzz_input_data = NULL;
    atomic_fetch_add(&fuzz_total_execs, 1);
}

#endif // FUZZ_WATCHDOG_H
