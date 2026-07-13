#define NOB_IMPLEMENTATION
#include "nob.h"
#include <sys/stat.h>

#define OUT "nvi"

#if defined(_WIN32) && defined(_MSC_VER)
#define OUT_BIN OUT ".exe"
#define BIN_EXT ".exe"
#else
#define OUT_BIN OUT
#define BIN_EXT ""
#endif

// Linux-only escape hatch: NVI_LIBC=musl builds a fully static, portable
// binary with musl-gcc (apt install musl-tools) instead of clang+glibc.
static bool use_musl(void) {
#if defined(__linux__)
    const char *v = getenv("NVI_LIBC");
    return v != NULL && strcmp(v, "musl") == 0;
#else
    return false;
#endif
}

// POSIX compiler selection: NVI_LIBC=musl wins (musl-gcc), then NVI_CC
// (eg. NVI_CC=gcc or NVI_CC=gcc-14), then clang. Fuzzing has its own
// override (FUZZ_CC) since it additionally requires the libFuzzer runtime.
static const char *posix_cc(void) {
    if (use_musl()) {
        return "musl-gcc";
    }

    const char *cc = getenv("NVI_CC");
    if (cc != NULL && cc[0] != '\0') {
        return cc;
    }

    return "clang";
}

// name sniff for release flag selection: compilers with "clang" in the name
// get the clang+lld release pipeline; anything else (gcc, musl-gcc) gets the
// conservative recipe, since -fuse-ld=lld and -Wl,--icf support vary by gcc
// version and linker
static bool posix_cc_is_clang(void) { return strstr(posix_cc(), "clang") != NULL; }

static const char *git_commit(void) {
    static const char *cached = NULL;
    if (cached != NULL) {
        return cached;
    }

    const char *env = getenv("NVI_COMMIT");
    if (env != NULL && env[0] != '\0') {
        cached = env;
        return cached;
    }

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "git", "rev-parse", "--short", "HEAD");
    if (!nob_cmd_run(&cmd, .stdout_path = ".nvi_commit")) {
        cached = "unknown";
        return cached;
    }

    Nob_String_Builder sb = {0};
    bool read_ok = nob_read_entire_file(".nvi_commit", &sb);
    nob_delete_file(".nvi_commit");
    if (!read_ok) {
        cached = "unknown";
        return cached;
    }

    while (sb.count > 0 && (sb.items[sb.count - 1] == '\n' || sb.items[sb.count - 1] == '\r')) {
        --sb.count;
    }
    nob_sb_append_null(&sb);

    cached = sb.items;
    return cached;
}

static void add_common_flags(Nob_Cmd *cmd, const char *build_label) {
    const char *commit_def = nob_temp_sprintf("-DNVI_COMMIT=\"%s\"", git_commit());
    const char *build_def = nob_temp_sprintf("-DNVI_BUILD=\"%s\"", build_label);

#if defined(_WIN32) && defined(_MSC_VER)
    nob_cmd_append(cmd, "cl", "/nologo", "/Isrc", commit_def, build_def, "/W4", "/std:c17", "/utf-8",
                   "/Zc:preprocessor");
#elif defined(__APPLE__) || defined(__linux__)
    nob_cmd_append(cmd, posix_cc(), "-Isrc", commit_def, build_def, "-Wformat-security", "-Wall",
                   "-Wextra", "-Wpedantic", "-std=gnu17", "-pthread");
#else
#error "unsupported platform (expected Windows/MSVC, macOS, or Linux)"
#endif
}

static void compose_dev_cmd(Nob_Cmd *cmd) {
    add_common_flags(cmd, "debug");

#if defined(_WIN32) && defined(_MSC_VER)
    nob_cmd_append(cmd, "/Zi", "/Od", "/Fe:" OUT_BIN);
#else
#if defined(__APPLE__) || defined(__linux__)
    // Match compose_test_cmd: sanitize dev builds so the integration suite
    // (which runs the dev binary) exercises the arena poisoning; musl skips
    // sanitizers (ASan targets glibc).
    if (!use_musl()) {
        nob_cmd_append(cmd, "-fsanitize=address,undefined", "-fno-omit-frame-pointer");
    }
#endif
    nob_cmd_append(cmd, "-g", "-O0", "-o", OUT_BIN);
#endif
}

static void compose_test_cmd(Nob_Cmd *cmd, const char *out_path) {
    add_common_flags(cmd, "test");

#if defined(_WIN32) && defined(_MSC_VER)
    nob_cmd_append(cmd, "/Itests/unity", "/Zi", "/Od", nob_temp_sprintf("/Fe:%s", out_path));
#else
    nob_cmd_append(cmd, "-Itests/unity", "-g", "-O0");
#if defined(__APPLE__) || defined(__linux__)
    // Sanitize unit tests on the dev platforms; MSVC is left unsanitized and
    // musl builds skip sanitizers (ASan targets glibc).
    if (!use_musl()) {
        nob_cmd_append(cmd, "-fsanitize=address,undefined", "-fno-omit-frame-pointer");
    }
#endif
    nob_cmd_append(cmd, "-o", out_path);
#endif
}

static bool collect_sources_except(const char *except, Nob_File_Paths *out) {
    Nob_File_Paths paths = {0};
    if (!nob_read_entire_dir("src", &paths)) {
        return false;
    }

    for (size_t i = 0; i < paths.count; ++i) {
        const char *name = paths.items[i];

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        size_t len = strlen(name);
        if (len < 2 || strcmp(name + len - 2, ".c") != 0) {
            continue;
        }

        if (except != NULL && strcmp(name, except) == 0) {
            continue;
        }

        nob_da_append(out, nob_temp_sprintf("src/%s", name));
    }

    nob_da_free(paths);
    return true;
}

static bool append_sources_except(Nob_Cmd *cmd, const char *except) {
    Nob_File_Paths srcs = {0};
    if (!collect_sources_except(except, &srcs)) {
        return false;
    }

    for (size_t i = 0; i < srcs.count; ++i) {
        nob_cmd_append(cmd, srcs.items[i]);
    }

    nob_da_free(srcs);
    return true;
}

static bool append_sources(Nob_Cmd *cmd) { return append_sources_except(cmd, NULL); }

static bool collect_test_names(Nob_File_Paths *out) {
    Nob_File_Paths paths = {0};
    if (!nob_read_entire_dir("tests", &paths)) {
        return false;
    }

    for (size_t i = 0; i < paths.count; ++i) {
        const char *name = paths.items[i];

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        size_t len = strlen(name);
        if (len < 2 || strcmp(name + len - 2, ".c") != 0) {
            continue;
        }

        nob_da_append(out, nob_temp_strdup(name));
    }

    nob_da_free(paths);
    return true;
}

static bool delete_entry(Nob_Walk_Entry e) { return nob_delete_file(e.path); }

static void remove_dir_recursively(const char *path) {
    if (nob_get_file_type(path) != NOB_FILE_DIRECTORY) {
        return;
    }
    nob_walk_dir(path, delete_entry, .post_order = true);
}

// Builds and runs a libFuzzer harness (POSIX + clang only; requires libFuzzer,
// which musl-gcc and MSVC don't ship). Usage: ./nob fuzz [parser|matcher|args|config].
// The target defaults to parser; remaining argv is forwarded to libFuzzer
// (eg. ./nob fuzz parser -runs=1000000, or a crash file to reproduce).
typedef struct {
    const char *name;    // target selector on the command line
    const char *harness; // harness source under tests/fuzz
    const char *corpus;  // per-target corpus directory
    const char *dict;    // optional libFuzzer dictionary (skipped when absent)
    bool seed_fixtures;  // seed the corpus from fixtures/*.env on first run
} fuzz_target_t;

static const fuzz_target_t fuzz_targets[] = {
    {
        .name = "parser",
        .harness = "tests/fuzz/fuzz_parser.c",
        .corpus = "build/fuzz/corpus",
        .dict = "tests/fuzz/env.dict",
        .seed_fixtures = true,
    },
    {
        .name = "matcher",
        .harness = "tests/fuzz/fuzz_matcher.c",
        .corpus = "build/fuzz/corpus-matcher",
        .dict = "tests/fuzz/matcher.dict",
        .seed_fixtures = false,
    },
    {
        .name = "args",
        .harness = "tests/fuzz/fuzz_args.c",
        .corpus = "build/fuzz/corpus-args",
        .dict = "tests/fuzz/args.dict",
        .seed_fixtures = false,
    },
    {
        .name = "config",
        .harness = "tests/fuzz/fuzz_config.c",
        .corpus = "build/fuzz/corpus-config",
        .dict = "tests/fuzz/config.dict",
        .seed_fixtures = false,
    },
};

static bool run_fuzz_target(const fuzz_target_t *target, int argc, char **argv) {
    if (!nob_mkdir_if_not_exists("build") || !nob_mkdir_if_not_exists("build/fuzz") ||
        !nob_mkdir_if_not_exists(target->corpus)) {
        return false;
    }

    // seed the corpus from fixtures once (libFuzzer mutates from there)
    if (target->seed_fixtures) {
        Nob_File_Paths fixtures = {0};
        if (nob_read_entire_dir("fixtures", &fixtures)) {
            for (size_t i = 0; i < fixtures.count; ++i) {
                const char *name = fixtures.items[i];
                size_t len = strlen(name);
                if (len < 4 || strcmp(name + len - 4, ".env") != 0) {
                    continue;
                }

                const char *dest = nob_temp_sprintf("%s/%s", target->corpus, name);
                if (nob_file_exists(dest) != 1) {
                    nob_copy_file(nob_temp_sprintf("fixtures/%s", name), dest);
                }
            }
            nob_da_free(fixtures);
        }
    }

    // seed from the tracked per-target directory holding fuzzer findings and
    // crafted regression inputs; unlike build/, these survive ./nob clean and
    // are replayed by every future run (including ./nob fuzz all -runs=0)
    const char *seeds_dir = nob_temp_sprintf("tests/fuzz/seeds/%s", target->name);
    if (nob_file_exists(seeds_dir) == 1) {
        Nob_File_Paths seeds = {0};
        if (nob_read_entire_dir(seeds_dir, &seeds)) {
            for (size_t i = 0; i < seeds.count; ++i) {
                const char *name = seeds.items[i];
                if (name[0] == '.') {
                    continue;
                }

                const char *dest = nob_temp_sprintf("%s/%s", target->corpus, name);
                if (nob_file_exists(dest) != 1) {
                    nob_copy_file(nob_temp_sprintf("%s/%s", seeds_dir, name), dest);
                }
            }
            nob_da_free(seeds);
        }
    }

    // Apple's clang ships asan/ubsan but not the libFuzzer runtime
    // (libclang_rt.fuzzer_osx.a), so on macOS prefer Homebrew LLVM's clang.
    // FUZZ_CC overrides compiler selection on any platform.
    const char *cc = getenv("FUZZ_CC");
#ifdef __APPLE__
    if (cc == NULL || cc[0] == '\0') {
        static const char *llvm_clangs[] = {
            "/opt/homebrew/opt/llvm/bin/clang", // arm64
            "/usr/local/opt/llvm/bin/clang",    // intel
        };

        for (size_t i = 0; i < NOB_ARRAY_LEN(llvm_clangs); ++i) {
            if (nob_file_exists(llvm_clangs[i]) == 1) {
                cc = llvm_clangs[i];
                break;
            }
        }

        if (cc == NULL) {
            nob_log(NOB_ERROR, "Apple clang does not ship the libFuzzer runtime; install Homebrew LLVM (brew "
                               "install llvm) or point FUZZ_CC at a libFuzzer-capable clang");
            return false;
        }
    }
#endif
    if (cc == NULL || cc[0] == '\0') {
        cc = "clang";
    }

    const char *out = nob_temp_sprintf("build/fuzz/fuzz_%s", target->name);

    // FUZZ_SAN overrides the sanitizer list (eg. FUZZ_SAN=fuzzer or
    // FUZZ_SAN=fuzzer,address) to bisect platform-specific sanitizer problems
    const char *san = getenv("FUZZ_SAN");
    if (san == NULL || san[0] == '\0') {
        san = "fuzzer,address,undefined";
    }

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, cc, "-Isrc", "-Wformat-security", "-Wall", "-Wextra", "-Wpedantic", "-std=gnu17", "-g", "-O1",
                   nob_temp_sprintf("-fsanitize=%s", san), "-fno-omit-frame-pointer", "-o", out);
    nob_cmd_append(&cmd, target->harness);
    if (!append_sources_except(&cmd, "main.c")) {
        return false;
    }

    if (!nob_cmd_run(&cmd)) {
        nob_log(NOB_ERROR, "failed to build the %s fuzz harness", target->name);
        return false;
    }

    Nob_Cmd run = {0};
    nob_cmd_append(&run, out, target->corpus,
                   // libFuzzer's own per-input timeout; the harness watchdog fires
                   // first (default 10s) and dumps the offending input
                   "-timeout=15", "-max_len=65536", "-print_final_stats=1");

    // seed libFuzzer's mutator with grammar tokens when the dictionary exists
    if (target->dict != NULL && nob_file_exists(target->dict) == 1) {
        nob_cmd_append(&run, nob_temp_sprintf("-dict=%s", target->dict));
    }

    for (int i = 0; i < argc; ++i) {
        nob_cmd_append(&run, argv[i]);
    }

    nob_log(NOB_INFO,
            "fuzzing %s; heartbeat every 5s (FUZZ_HEARTBEAT_SECONDS), stall limit 10s "
            "(FUZZ_STALL_SECONDS); ctrl-c to stop",
            target->name);

    return nob_cmd_run(&run);
}

// Usage: ./nob fuzz [parser|matcher|args|config|all] [libFuzzer args]. The target
// defaults to parser; 'all' runs every target sequentially with the same
// forwarded args (eg. ./nob fuzz all -runs=0 replays every corpus in CI).
static bool run_fuzz(int argc, char **argv) {
#if defined(_WIN32) && defined(_MSC_VER)
    (void)argc;
    (void)argv;
    nob_log(NOB_ERROR, "fuzzing requires clang + libFuzzer and is not supported under MSVC");
    return false;
#else
    if (use_musl()) {
        nob_log(NOB_ERROR, "fuzzing requires clang + libFuzzer; unset NVI_LIBC=musl");
        return false;
    }

    // an optional leading target name selects the harness; default is parser
    const fuzz_target_t *target = &fuzz_targets[0];
    bool run_all = false;
    if (argc > 0) {
        if (strcmp(argv[0], "all") == 0) {
            run_all = true;
            nob_shift(argv, argc);
        } else {
            for (size_t i = 0; i < NOB_ARRAY_LEN(fuzz_targets); ++i) {
                if (strcmp(argv[0], fuzz_targets[i].name) == 0) {
                    target = &fuzz_targets[i];
                    nob_shift(argv, argc);
                    break;
                }
            }
        }
    }

    if (!run_all) {
        return run_fuzz_target(target, argc, argv);
    }

    // an unbounded 'all' would fuzz the first target forever and never reach
    // the rest, so require a libFuzzer flag that guarantees termination
    bool bounded = false;
    for (int i = 0; i < argc; ++i) {
        if (strncmp(argv[i], "-runs=", 6) == 0 || strncmp(argv[i], "-max_total_time=", 16) == 0) {
            bounded = true;
            break;
        }
    }

    if (!bounded) {
        nob_log(NOB_ERROR, "'fuzz all' requires a bound so every target gets a turn; add -runs=<N> (eg. -runs=0 to "
                           "replay the corpora) or -max_total_time=<seconds>");
        return false;
    }

    size_t passed = 0;
    for (size_t i = 0; i < NOB_ARRAY_LEN(fuzz_targets); ++i) {
        if (run_fuzz_target(&fuzz_targets[i], argc, argv)) {
            ++passed;
        } else {
            nob_log(NOB_ERROR, "fuzz target '%s' failed", fuzz_targets[i].name);
        }
    }

    nob_log(passed == NOB_ARRAY_LEN(fuzz_targets) ? NOB_INFO : NOB_ERROR, "fuzz targets: %zu/%zu passed", passed,
            NOB_ARRAY_LEN(fuzz_targets));
    return passed == NOB_ARRAY_LEN(fuzz_targets);
#endif
}

static bool build_dev(void) {
    Nob_Cmd cmd = {0};
    compose_dev_cmd(&cmd);

    if (!append_sources(&cmd)) {
        return false;
    }

    return nob_cmd_run(&cmd);
}

static bool build_release(void) {
    Nob_Cmd cmd = {0};
    add_common_flags(&cmd, "release");

#if defined(_WIN32) && defined(_MSC_VER)
    nob_cmd_append(&cmd, "/DNDEBUG");
#else
    nob_cmd_append(&cmd, "-DNDEBUG");
#endif

#if defined(_WIN32) && defined(_MSC_VER)
    nob_cmd_append(&cmd, "/O1", "/GL", "/Gy", "/Gw", "/Fe:" OUT_BIN);
#elif defined(__APPLE__)
    nob_cmd_append(&cmd, "-Oz", "-flto", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables",
                   "-Wl,-dead_strip_dylibs", "-Wl,-dead_strip", "-Wl,-x", "-o", OUT_BIN);
#elif defined(__linux__)
    if (!posix_cc_is_clang()) {
        // gcc-family recipe (musl-gcc or NVI_CC=gcc): gcc has no -flto parity
        // with the clang+lld pipeline here, and -fuse-ld=lld/--icf support
        // varies by gcc version, so keep the plain verified size flags.
        // musl keeps its verified -Oz (needs gcc >= 12) and links fully
        // static; NVI_CC=gcc falls back to -Os so distro compilers as old as
        // gcc 11 still build (gcc only learned -Oz in 12)
        nob_cmd_append(&cmd, use_musl() ? "-Oz" : "-Os", "-fno-ident", "-fno-unwind-tables",
                       "-fno-asynchronous-unwind-tables", "-ffunction-sections", "-fdata-sections",
                       "-Wl,--gc-sections", "-s");
        if (use_musl()) {
            nob_cmd_append(&cmd, "-static");
        }
        nob_cmd_append(&cmd, "-o", OUT_BIN);
    } else {
        const char *mps = getenv("NVI_MAX_PAGE_SIZE");
        // Page sizing breakdown:
        // 0x1000 = 4096 = 4KB
        // 0x4000 = 16384 = 16KB
        // 0x10000 = 65536 = 64KB (default)
        if (mps == NULL || mps[0] == '\0') {
            mps = "0x10000";
        }
        // lld lays out the (few) segments far more compactly than BFD ld under
        // these flags and enables identical-code folding; requires the lld
        // package alongside clang
        nob_cmd_append(&cmd, "-Oz", "-flto", "-fno-ident", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables",
                       "-ffunction-sections", "-fdata-sections", "-fuse-ld=lld", "-Wl,--icf=all", "-Wl,--gc-sections",
                       "-Wl,-z,noseparate-code", "-Wl,--build-id=none",
                       nob_temp_sprintf("-Wl,-z,max-page-size=%s", mps), "-s", "-o", OUT_BIN);
    }
#endif

    if (!append_sources(&cmd)) {
        return false;
    }

#if defined(_WIN32) && defined(_MSC_VER)
    // everything after /link goes to the linker: be explicit about LTCG and
    // fold/strip unreferenced COMDATs produced by /Gy and /Gw
    nob_cmd_append(&cmd, "/link", "/LTCG", "/OPT:REF", "/OPT:ICF");
#endif

    if (!nob_cmd_run(&cmd)) {
        return false;
    }

    return true;
}

static bool build_one_test(const char *test_src, const char *out_path) {
    Nob_Cmd cmd = {0};
    compose_test_cmd(&cmd, out_path);

    nob_cmd_append(&cmd, test_src, "tests/unity/unity.c");

    if (!append_sources_except(&cmd, "main.c")) {
        return false;
    }

    return nob_cmd_run(&cmd);
}

static bool run_tests(void) {
    if (!nob_mkdir_if_not_exists("build") || !nob_mkdir_if_not_exists("build/tests")) {
        return false;
    }

    Nob_File_Paths tests = {0};
    if (!collect_test_names(&tests)) {
        return false;
    }

    size_t total = 0;
    size_t passed = 0;

    for (size_t i = 0; i < tests.count; ++i) {
        const char *name = tests.items[i];
        size_t len = strlen(name);

        const char *stem = nob_temp_sprintf("%.*s", (int)(len - 2), name);
        const char *src = nob_temp_sprintf("tests/%s", name);
        const char *out = nob_temp_sprintf("build/tests/%s" BIN_EXT, stem);

        ++total;

        if (!build_one_test(src, out)) {
            nob_log(NOB_ERROR, "failed to build %s", stem);
            continue;
        }

        Nob_Cmd run = {0};
        nob_cmd_append(&run, out);
        if (nob_cmd_run(&run)) {
            ++passed;
        } else {
            nob_log(NOB_ERROR, "%s reported failures", stem);
        }
    }

    nob_da_free(tests);

    nob_log(passed == total ? NOB_INFO : NOB_ERROR, "test suites: %zu/%zu passed", passed, total);
    return passed == total;
}

static bool build_integration_runner(const char *out_path) {
    Nob_Cmd cmd = {0};

#if defined(_WIN32) && defined(_MSC_VER)
    nob_cmd_append(&cmd, "cl", "/nologo", "/W4", "/std:c17", "/utf-8", "/Zi", "/Od",
                   nob_temp_sprintf("/Fe:%s", out_path));
#else
    nob_cmd_append(&cmd, posix_cc(), "-Wformat-security", "-Wall", "-Wextra", "-Wpedantic", "-std=gnu17", "-g", "-O0",
                   "-o", out_path);
#endif

    nob_cmd_append(&cmd, "tests/integration/main.c");

    return nob_cmd_run(&cmd);
}

static bool run_integration(void) {
    if (!nob_mkdir_if_not_exists("build") || !nob_mkdir_if_not_exists("build/tests")) {
        return false;
    }

    if (!build_dev()) {
        nob_log(NOB_ERROR, "failed to build " OUT_BIN " for integration tests");
        return false;
    }

    const char *runner = "build/tests/integration-runner" BIN_EXT;
    if (!build_integration_runner(runner)) {
        nob_log(NOB_ERROR, "failed to build the integration runner");
        return false;
    }

    Nob_Cmd run = {0};
    nob_cmd_append(&run, runner);
    if (!nob_cmd_run(&run)) {
        nob_log(NOB_ERROR, "integration tests reported failures");
        return false;
    }

    nob_log(NOB_INFO, "All integration tests passed");
    return true;
}

static int cmp_cstr(const void *a, const void *b) { return strcmp(*(const char *const *)a, *(const char *const *)b); }

static void json_str(Nob_String_Builder *sb, const char *s) {
    nob_da_append(sb, '"');
    for (const char *p = s; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        switch (c) {
            case '"':
                nob_sb_append_cstr(sb, "\\\"");
                break;
            case '\\':
                nob_sb_append_cstr(sb, "\\\\");
                break;
            case '\b':
                nob_sb_append_cstr(sb, "\\b");
                break;
            case '\f':
                nob_sb_append_cstr(sb, "\\f");
                break;
            case '\n':
                nob_sb_append_cstr(sb, "\\n");
                break;
            case '\r':
                nob_sb_append_cstr(sb, "\\r");
                break;
            case '\t':
                nob_sb_append_cstr(sb, "\\t");
                break;
            default:
                if (c < 0x20) {
                    nob_sb_appendf(sb, "\\u%04x", c);
                } else {
                    nob_da_append(sb, (char)c);
                }
        }
    }
    nob_da_append(sb, '"');
}

static void emit_entry(Nob_String_Builder *json, bool *first, const char *dir, const Nob_Cmd *prefix,
                       const char *output, const char *src) {
    if (!*first) {
        nob_sb_append_cstr(json, ",\n");
    }
    *first = false;

    nob_sb_append_cstr(json, "  {\n    \"directory\": ");
    json_str(json, dir);
    nob_sb_append_cstr(json, ",\n    \"file\": ");
    json_str(json, src);
    nob_sb_append_cstr(json, ",\n    \"output\": ");
    json_str(json, output);
    nob_sb_append_cstr(json, ",\n    \"arguments\": [");
    for (size_t i = 0; i < prefix->count; ++i) {
        if (i) {
            nob_sb_append_cstr(json, ", ");
        }
        json_str(json, prefix->items[i]);
    }
    if (prefix->count) {
        nob_sb_append_cstr(json, ", ");
    }
    json_str(json, src);
    nob_sb_append_cstr(json, "]\n  }");
}

static bool cmd_generate(void) {
    const char *dir = nob_get_current_dir_temp();

    Nob_String_Builder json = {0};
    nob_sb_append_cstr(&json, "[\n");
    bool first = true;
    size_t count = 0;

    {
        Nob_Cmd prefix = {0};
        compose_dev_cmd(&prefix);

        Nob_File_Paths srcs = {0};
        if (!collect_sources_except(NULL, &srcs)) {
            nob_cmd_free(prefix);
            nob_sb_free(json);
            return false;
        }
        qsort(srcs.items, srcs.count, sizeof(*srcs.items), cmp_cstr);

        for (size_t i = 0; i < srcs.count; ++i) {
            emit_entry(&json, &first, dir, &prefix, OUT_BIN, srcs.items[i]);
            ++count;
        }

        nob_da_free(srcs);
        nob_cmd_free(prefix);
    }

    {
        Nob_Cmd prefix = {0};
#if defined(_WIN32) && defined(_MSC_VER)
        nob_cmd_append(&prefix, "cl", "/nologo", "/W4", "/std:c17", "/utf-8", "/Zi", "/Od");
#else
        nob_cmd_append(&prefix, "clang", "-Wformat-security", "-Wall", "-Wextra", "-Wpedantic", "-std=gnu17", "-g",
                       "-O0");
#endif
        emit_entry(&json, &first, dir, &prefix, "build/tests/integration-runner" BIN_EXT, "tests/integration/main.c");
        ++count;
        nob_cmd_free(prefix);
    }

    Nob_File_Paths lib = {0};
    if (!collect_sources_except("main.c", &lib)) {
        nob_sb_free(json);
        return false;
    }
    qsort(lib.items, lib.count, sizeof(*lib.items), cmp_cstr);

    Nob_File_Paths tests = {0};
    if (!collect_test_names(&tests)) {
        nob_da_free(lib);
        nob_sb_free(json);
        return false;
    }
    qsort(tests.items, tests.count, sizeof(*tests.items), cmp_cstr);

    for (size_t t = 0; t < tests.count; ++t) {
        const char *name = tests.items[t];
        size_t len = strlen(name);
        const char *stem = nob_temp_sprintf("%.*s", (int)(len - 2), name);
        const char *src = nob_temp_sprintf("tests/%s", name);
        const char *out = nob_temp_sprintf("build/tests/%s" BIN_EXT, stem);

        Nob_Cmd prefix = {0};
        compose_test_cmd(&prefix, out);

        emit_entry(&json, &first, dir, &prefix, out, src);
        ++count;
        emit_entry(&json, &first, dir, &prefix, out, "tests/unity/unity.c");
        ++count;
        for (size_t i = 0; i < lib.count; ++i) {
            emit_entry(&json, &first, dir, &prefix, out, lib.items[i]);
            ++count;
        }

        nob_cmd_free(prefix);
    }

    nob_da_free(tests);
    nob_da_free(lib);

    nob_sb_append_cstr(&json, "\n]\n");

    bool ok = nob_write_entire_file("compile_commands.json", json.items, json.count);
    if (ok) {
        nob_log(NOB_INFO, "wrote compile_commands.json (%zu entries)", count);
    }

    nob_sb_free(json);
    return ok;
}

static bool timed(const char *label, const char *out, bool (*build)(void)) {
    uint64_t start = nob_nanos_since_unspecified_epoch();
    bool ok = build();
    uint64_t elapsed = nob_nanos_since_unspecified_epoch() - start;

    nob_log(ok ? NOB_INFO : NOB_ERROR, "%s build %s in %.3f s", label, ok ? "succeeded" : "failed",
            (double)elapsed / NOB_NANOS_PER_SEC);

    if (ok) {
        struct stat st;
        if (stat(out, &st) != 0) {
            nob_log(NOB_WARNING, "could not stat %s for size", out);
            return false;
        }
        nob_log(NOB_INFO, "%s is %.1f KB (%lld bytes)", out, (double)st.st_size / 1024.0, (long long)st.st_size);
    }

    return ok;
}

int main(int argc, char **argv) {
#if !defined(_WIN32) || !defined(_MSC_VER)
    NOB_GO_REBUILD_URSELF(argc, argv);
#endif

    // drop program name
    nob_shift(argv, argc);

    const char *subcmd = argc > 0 ? nob_shift(argv, argc) : "dev";

    if (strcmp(subcmd, "install") == 0) {
        const char *dir = argc > 0 ? nob_shift(argv, argc) : NULL;
        if (dir == NULL || dir[0] == '\0') {
            nob_log(NOB_ERROR, "installation directory was not set; run: ./nob install <path>");
            return 1;
        }

        if (!timed("release", OUT_BIN, build_release)) {
            return 1;
        }

        const char *dest = nob_temp_sprintf("%s/%s", dir, OUT_BIN);

        if (!nob_copy_file(OUT_BIN, dest)) {
            nob_log(NOB_ERROR, "failed to copy %s -> %s (permission denied? try a directory owned by you)", OUT_BIN,
                    dest);
            return 1;
        }

        nob_log(NOB_INFO, "installed %s to %s", OUT_BIN, dest);
    } else if (strcmp(subcmd, "release") == 0) {
        if (!timed("release", OUT_BIN, build_release)) {
            return 1;
        }
    } else if (strcmp(subcmd, "test") == 0) {
        if (!cmd_generate()) {
            return 1;
        }

        if (!run_tests()) {
            return 1;
        }

        if (!run_integration()) {
            return 1;
        }
    } else if (strcmp(subcmd, "unit") == 0) {
        if (!cmd_generate()) {
            return 1;
        }

        if (!run_tests()) {
            return 1;
        }
    } else if (strcmp(subcmd, "integration") == 0) {
        if (!run_integration()) {
            return 1;
        }
    } else if (strcmp(subcmd, "fuzz") == 0) {
        if (!run_fuzz(argc, argv)) {
            return 1;
        }
    } else if (strcmp(subcmd, "generate") == 0) {
        if (!cmd_generate()) {
            return 1;
        }
    } else if (strcmp(subcmd, "clean") == 0) {
        nob_delete_file(OUT_BIN);
        nob_delete_file("compile_commands.json");
        remove_dir_recursively("build/tests");
        remove_dir_recursively("build");
#ifdef __APPLE__
        remove_dir_recursively(OUT_BIN ".dSYM");
#endif
    } else {
        if (!timed("dev", OUT_BIN, build_dev)) {
            return 1;
        }
    }

    return 0;
}
