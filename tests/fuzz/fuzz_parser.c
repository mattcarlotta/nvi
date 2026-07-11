// libFuzzer harness for the tokenizer -> parser pipeline (POSIX only).
//
// Feeds arbitrary bytes through generate_tokens and run_parser as if they were
// the contents of a single .env file. No filesystem access, fresh arenas per
// input, so every iteration is hermetic.
//
// Progress visibility (the old fuzzer looked hung):
// 1. A watchdog thread prints a heartbeat every FUZZ_HEARTBEAT_SECONDS (default 5)
//    with total execs, execs/sec since the last beat, and the current input's
//    runtime. It writes to a dup of the real stderr grabbed in a constructor,
//    so it stays visible even when the run uses -close_fd_mask=3 to silence
//    the parser's own [ERROR] spam.
// 2. If a single input runs longer than FUZZ_STALL_SECONDS (default 10), the
//    watchdog dumps it to fuzz-stall-<pid>.bin and aborts, so a genuine hang
//    becomes a reproducible artifact instead of a silent stall.
//
// Build/run: ./nob fuzz [extra libFuzzer flags]

#include "arena.h"
#include "arg.h"
#include "file.h"
#include "parser.h"
#include "result.h"
#include "tokenizer.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int heartbeat_fd = -1;
static _Atomic uint64_t total_execs = 0;
static _Atomic uint64_t input_started_ns = 0; // 0 when no input is in flight
static _Atomic size_t input_len = 0;
static const uint8_t *volatile input_data = NULL;

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

// grab the real stderr before libFuzzer can close/redirect fd 2 (-close_fd_mask)
__attribute__((constructor)) static void grab_heartbeat_fd(void) { heartbeat_fd = dup(STDERR_FILENO); }

static void heartbeat_printf(const char *fmt, ...) {
    if (heartbeat_fd < 0) {
        return;
    }

    char line[256];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    if (n > 0) {
        ssize_t unused = write(heartbeat_fd, line, (size_t)n < sizeof(line) ? (size_t)n : sizeof(line) - 1);
        (void)unused;
    }
}

static unsigned env_seconds(const char *name, unsigned fallback) {
    const char *v = getenv(name);
    if (v == NULL || v[0] == '\0') {
        return fallback;
    }

    char *end = NULL;
    unsigned long n = strtoul(v, &end, 10);
    return (end != NULL && *end == '\0' && n > 0 && n < 3600) ? (unsigned)n : fallback;
}

static void dump_stall_input(void) {
    const uint8_t *data = input_data;
    size_t len = atomic_load(&input_len);
    if (data == NULL) {
        return;
    }

    char path[64];
    snprintf(path, sizeof(path), "fuzz-stall-%d.bin", (int)getpid());

    FILE *f = fopen(path, "wb");
    if (f != NULL) {
        fwrite(data, 1, len, f);
        fclose(f);
        heartbeat_printf("[fuzz] stalled input written to %s (%zu bytes)\n", path, len);
    }
}

static void *watchdog_main(void *unused) {
    (void)unused;

    const unsigned beat_s = env_seconds("FUZZ_HEARTBEAT_SECONDS", 5);
    const unsigned stall_s = env_seconds("FUZZ_STALL_SECONDS", 10);
    const uint64_t started = now_ns();
    uint64_t prev_execs = 0;

    for (;;) {
        struct timespec ts = {.tv_sec = beat_s};
        nanosleep(&ts, NULL);

        uint64_t execs = atomic_load(&total_execs);
        uint64_t in_flight = atomic_load(&input_started_ns);
        uint64_t now = now_ns();

        double elapsed_s = (double)(now - started) / 1e9;
        double rate = (double)(execs - prev_execs) / (double)beat_s;
        prev_execs = execs;

        if (in_flight != 0) {
            double input_s = (double)(now - in_flight) / 1e9;

            if (input_s >= (double)stall_s) {
                heartbeat_printf("[fuzz] STALL: current input (%zu bytes) has run %.1fs (limit %us); aborting\n",
                                 atomic_load(&input_len), input_s, stall_s);
                dump_stall_input();
                abort();
            }

            heartbeat_printf("[fuzz] alive: execs=%llu (%.0f/s) elapsed=%.0fs current_input=%zu bytes (%.1fs)\n",
                             (unsigned long long)execs, rate, elapsed_s, atomic_load(&input_len), input_s);
        } else {
            heartbeat_printf("[fuzz] alive: execs=%llu (%.0f/s) elapsed=%.0fs idle\n", (unsigned long long)execs, rate,
                             elapsed_s);
        }
    }

    return NULL;
}

int LLVMFuzzerInitialize(int *argc, char ***argv) {
    (void)argc;
    (void)argv;

    heartbeat_printf("[fuzz] tokenizer->parser harness up; heartbeat every %us, stall limit %us\n",
                     env_seconds("FUZZ_HEARTBEAT_SECONDS", 5), env_seconds("FUZZ_STALL_SECONDS", 10));

    // The tokenizer/parser print an [ERROR] line to stderr for most mutated
    // inputs, which drowns libFuzzer's status output. Reassign the stderr
    // global to /dev/null: errors.c reads the global per call so its spam is
    // discarded, while libFuzzer captured the original FILE at static init and
    // ASan writes crash reports to fd 2 directly, so both stay visible.
    // FUZZ_VERBOSE keeps the spam for debugging a single reproducer.
    if (getenv("FUZZ_VERBOSE") == NULL) {
        FILE *devnull = fopen("/dev/null", "w");
        if (devnull != NULL) {
            stderr = devnull;
        }
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, watchdog_main, NULL) == 0) {
        pthread_detach(tid);
    }

    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size > MAX_FILE_SIZE) {
        return 0;
    }

    input_data = data;
    atomic_store(&input_len, size);
    atomic_store(&input_started_ns, now_ns());

    arena_t arena = {0};
    arena_t scratch = {0};
    args_t args = {0};
    tokenizer_t tokenizer = {0};
    parser_t parser = {0};

    // mirror open_file: contents are NUL-terminated, len excludes the NUL
    char *contents = arena_alloc(&arena, size + 1);
    memcpy(contents, data, size);
    contents[size] = '\0';

    const file_details_t file = {.contents = contents, .path = "fuzz.env", .len = size};

    result_t result = generate_tokens(&arena, &scratch, &args, &file, &tokenizer);
    if (result.ok) {
        run_parser(&arena, &args, &tokenizer.tokens, &parser);
    }

    arena_free(&scratch);
    arena_free(&arena);

    atomic_store(&input_started_ns, 0);
    input_data = NULL;
    atomic_fetch_add(&total_execs, 1);

    return 0;
}
