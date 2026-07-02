// Run via `./nob integration` (or as part of `./nob test`), which builds the
// binary first and executes this runner from the repository root.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) && defined(_MSC_VER)
#include <direct.h>
#define chdir _chdir
#define make_dir(path) _mkdir(path)
#define NVI_BIN "nvi.exe"
#define NVI_FROM_SCANROOT "..\\..\\..\\nvi.exe"
static void set_env(const char *k, const char *v) { _putenv_s(k, v); }
#else
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#define make_dir(path) mkdir((path), 0755)
#define NVI_BIN "./nvi"
#define NVI_FROM_SCANROOT "../../../nvi"
static void set_env(const char *k, const char *v) { setenv(k, v, 1); }
#endif

#define IT_DIR "build/it"

// stdout/stderr capture paths, swapped when the scanner cases chdir into the
// scratch tree (relative paths would otherwise dangle)
static const char *out_path = IT_DIR "/stdout.txt";
static const char *err_path = IT_DIR "/stderr.txt";

static size_t total = 0;
static size_t failed = 0;

static void write_file(const char *path, const void *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        fprintf(stderr, "[ERROR] cannot create test input '%s'\n", path);
        exit(1);
    }
    fwrite(data, 1, len, f);
    fclose(f);
}

static size_t read_file(const char *path, char *out, size_t cap) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return 0;
    }
    size_t n = fread(out, 1, cap - 1, f);
    fclose(f);
    out[n] = '\0';
    return n;
}

static int run_nvi(const char *bin, const char *args) {
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "%s %s >%s 2>%s", bin, args, out_path, err_path);
    int rc = system(cmd);
#if !defined(_WIN32)
    rc = WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
#endif
    return rc;
}

static void print_bytes(const char *label, const char *s, size_t len) {
    fprintf(stderr, "  %s (%zu bytes): ", label, len);
    for (size_t i = 0; i < len && i < 160; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\0') {
            fputs("\\0", stderr);
        } else if (c == '\n') {
            fputs("\\n", stderr);
        } else if (c == '\r') {
            fputs("\\r", stderr);
        } else {
            fputc(c, stderr);
        }
    }
    fputc('\n', stderr);
}

static void check(const char *name, const char *bin, const char *args, int exit_code, const char *expected_stdout,
                  size_t expected_stdout_len, const char *stderr_contains) {
    ++total;
    int rc = run_nvi(bin, args);

    static char out[65536];
    static char err[65536];
    size_t out_len = read_file(out_path, out, sizeof(out));
    size_t err_len = read_file(err_path, err, sizeof(err));
    (void)err_len;

    bool ok = rc == exit_code;
    if (ok && expected_stdout != NULL) {
        ok = out_len == expected_stdout_len && memcmp(out, expected_stdout, expected_stdout_len) == 0;
    }
    if (ok && stderr_contains != NULL) {
        ok = strstr(err, stderr_contains) != NULL;
    }

    if (ok) {
        printf("ok %zu - %s\n", total, name);
        return;
    }

    ++failed;
    printf("not ok %zu - %s\n", total, name);
    fprintf(stderr, "  args: %s\n", args);
    fprintf(stderr, "  exit: expected %d, got %d\n", exit_code, rc);
    if (expected_stdout != NULL) {
        print_bytes("expected stdout", expected_stdout, expected_stdout_len);
        print_bytes("actual stdout  ", out, out_len);
    }
    if (stderr_contains != NULL) {
        fprintf(stderr, "  stderr must contain: %s\n", stderr_contains);
        print_bytes("actual stderr  ", err, strlen(err));
    }
}

#define EXPECT(lit) (lit), sizeof(lit) - 1
#define NO_STDOUT NULL, 0

static void setup_inputs(void) {
    make_dir("build");
    make_dir(IT_DIR);
    make_dir(IT_DIR "/scanroot");

    write_file(IT_DIR "/a.env", EXPECT("MESSAGE=hello\nGREETING=${MESSAGE} world\n"));
    write_file(IT_DIR "/b.env", EXPECT("MESSAGE=goodbye\n"));
    write_file(IT_DIR "/quote.env", EXPECT("MSG=it's\n"));
    write_file(IT_DIR "/crlf.env", EXPECT("A=1\r\nB=2\r\n"));
    write_file(IT_DIR "/bom.env", EXPECT("\xEF\xBB\xBFKEY=value\n"));
    write_file(IT_DIR "/multi.env", EXPECT("MULTI=line1\\\nline2\n"));
    write_file(IT_DIR "/literals.env", EXPECT("PRICE=$5.00\nCHANNEL=#general\nBASE64=abc==\n"));
    write_file(IT_DIR "/shell_interp.env", EXPECT("X=${NVI_IT_FROM_SHELL}\n"));
    write_file(IT_DIR "/empty.env", "", 0);
    write_file(IT_DIR "/bad_interp.env", EXPECT("BAD=${OPEN\n"));
    write_file(IT_DIR "/empty_key.env", EXPECT("=ABC\n"));
    write_file(IT_DIR "/required.env", EXPECT("API_KEY=abc\n"));

    write_file(IT_DIR "/quoted.env", EXPECT("DQ=\"hello world\"\nSQ='keep ${LIT}'\nEMPTYQ=\"\"\n"));
    write_file(IT_DIR "/export.env", EXPECT("export EXPORTED=value\n"));
    write_file(IT_DIR "/fallback.env", EXPECT("FB=${NVI_IT_NOT_SET:-fell back}\n"));
    write_file(IT_DIR "/undef.env", EXPECT("U=${NVI_IT_DEFINITELY_NOT_SET}\n"));
    write_file(IT_DIR "/badkey.env", EXPECT("MY KEY=1\n"));
    write_file(IT_DIR "/req_empty.env", EXPECT("EMPTYV=${NVI_IT_UNSET_EMPTY:-}\n"));
    make_dir(IT_DIR "/dir.env");

    write_file(IT_DIR "/scanroot/it.env", EXPECT("IT_SCAN_KEY=1\n"));
    write_file(IT_DIR "/scanroot/partial.env", EXPECT("UNRELATED=1\n"));
    write_file(IT_DIR "/scanroot/src.ts", EXPECT("const k = process.env.IT_SCAN_KEY;\n"));
}

int main(void) {
    setup_inputs();

    // --- emission ---

    check("emits nul pairs then the command", NVI_BIN, "--files build/it/a.env -F nul -- echo hi", 0,
          EXPECT("MESSAGE=hello\0GREETING=hello world\0echo\0hi\0"), NULL);

    check("later files override earlier ones in place", NVI_BIN, "--files build/it/a.env build/it/b.env -F nul -- x", 0,
          EXPECT("MESSAGE=goodbye\0GREETING=hello world\0x\0"), NULL);

    check("powershell format escapes quotes and prefixes the call operator", NVI_BIN,
          "--files build/it/quote.env -F powershell -- echo hi", 0, EXPECT("$env:MSG = 'it''s'\n& 'echo' 'hi'\n"),
          NULL);

    check("emits nothing to stdout without a command", NVI_BIN, "--files build/it/a.env -F nul", 0, EXPECT(""), NULL);

    // --- input robustness ---

    check("crlf line endings do not leak into values", NVI_BIN, "--files build/it/crlf.env -F nul -- x", 0,
          EXPECT("A=1\0B=2\0x\0"), NULL);

    check("utf-8 bom does not leak into the first key", NVI_BIN, "--files build/it/bom.env -F nul -- x", 0,
          EXPECT("KEY=value\0x\0"), NULL);

    check("backslash-newline joins a multiline value", NVI_BIN, "--files build/it/multi.env -F nul -- x", 0,
          EXPECT("MULTI=line1line2\0x\0"), NULL);

    check("bare '$', '#' after a key, and '=' in values are literal", NVI_BIN,
          "--files build/it/literals.env -F nul -- x", 0, EXPECT("PRICE=$5.00\0CHANNEL=#general\0BASE64=abc==\0x\0"),
          NULL);

    check("surrounding quotes are stripped and single quotes block interpolation", NVI_BIN,
          "--files build/it/quoted.env -F nul -- x", 0, EXPECT("DQ=hello world\0SQ=keep ${LIT}\0EMPTYQ=\0x\0"), NULL);

    check("a shell-style export prefix is stripped from keys", NVI_BIN, "--files build/it/export.env -F nul -- x", 0,
          EXPECT("EXPORTED=value\0x\0"), NULL);

    check("an unset interpolation falls back to its ':-' default", NVI_BIN, "--files build/it/fallback.env -F nul -- x",
          0, EXPECT("FB=fell back\0x\0"), NULL);

    set_env("NVI_IT_FROM_SHELL", "fromshell");
    check("interpolation resolves from the process environment", NVI_BIN,
          "--files build/it/shell_interp.env -F nul -- x", 0, EXPECT("X=fromshell\0x\0"), NULL);

    // --- operational errors (exit 1) ---

    check("an empty --files file is a loud error", NVI_BIN, "--files build/it/empty.env -- x", 1, NO_STDOUT,
          "is empty");

    check("a missing --files file is a loud error", NVI_BIN, "--files build/it/missing.env -- x", 1, NO_STDOUT,
          "Cannot open");

    check("an unterminated interpolation is a tokenizer error", NVI_BIN, "--files build/it/bad_interp.env -- x", 1,
          NO_STDOUT, "unterminated value interpolation");

    check("an assignment without a key is a tokenizer error", NVI_BIN, "--files build/it/empty_key.env -- x", 1,
          NO_STDOUT, "without a key name");

    check("an undefined interpolation without a fallback is a loud error", NVI_BIN, "--files build/it/undef.env -- x",
          1, NO_STDOUT, "not defined");

    check("an invalid key name is a tokenizer error", NVI_BIN, "--files build/it/badkey.env -- x", 1, NO_STDOUT,
          "not a valid ENV name");

    check("a required key that resolves empty is a loud error", NVI_BIN,
          "--files build/it/req_empty.env --required EMPTYV -- x", 1, NO_STDOUT, "EMPTYV");

    check("a directory masquerading as an env file is a loud error", NVI_BIN, "--files build/it/dir.env -- x", 1,
          NO_STDOUT, "not a regular file");

    check("a missing required key is a loud error", NVI_BIN,
          "--files build/it/required.env --required DATABASE_URL -- x", 1, NO_STDOUT, "DATABASE_URL");

    check("a present required key passes", NVI_BIN, "--files build/it/required.env --required API_KEY -F nul -- x", 0,
          EXPECT("API_KEY=abc\0x\0"), NULL);

    // --- usage errors (exit 2) ---

    check("missing --files and --scan is a usage error", NVI_BIN, "--dry-run", 2, NO_STDOUT, "requires at least one");

    check("an unknown flag is a usage error", NVI_BIN, "--files build/it/a.env --nope", 2, NO_STDOUT,
          "Unrecognized flag");

    // --- info commands ---

    check("help prints usage to stdout and exits 0", NVI_BIN, "--help", 0, NULL, 0, NULL);
    check("version exits 0", NVI_BIN, "--version", 0, NULL, 0, NULL);

    check("dry run keeps stdout empty and reports to stderr", NVI_BIN, "--files build/it/a.env --dry-run", 0,
          EXPECT(""), "Dry run completed");

    // --- scanner (runs relative to cwd, so hop into the scratch tree) ---

    if (chdir(IT_DIR "/scanroot") != 0) {
        fprintf(stderr, "[ERROR] cannot chdir into " IT_DIR "/scanroot\n");
        return 1;
    }
    out_path = "it_stdout.txt";
    err_path = "it_stderr.txt";

    check("scan-required key satisfied by the .env passes", NVI_FROM_SCANROOT, "--scan ts --files it.env -F nul -- x",
          0, EXPECT("IT_SCAN_KEY=1\0x\0"), NULL);

    check("scan-required key missing from the .env fails", NVI_FROM_SCANROOT, "--scan ts --files partial.env -- x", 1,
          NO_STDOUT, "IT_SCAN_KEY");

    check("scan-required key rescued by --ignored passes", NVI_FROM_SCANROOT,
          "--scan ts --ignored IT_SCAN_KEY --files partial.env -F nul -- x", 0, EXPECT("UNRELATED=1\0x\0"), NULL);

#if !defined(_WIN32)
    // a symlink cycle must be skipped, not followed to death
    (void)system("ln -sfn .. loop");
    check("symlinked directories are not followed", NVI_FROM_SCANROOT, "--scan ts --files it.env -F nul -- x", 0,
          EXPECT("IT_SCAN_KEY=1\0x\0"), NULL);
    (void)system("rm -f loop");
#endif

    if (chdir("../../..") != 0) {
        fprintf(stderr, "[ERROR] cannot chdir back to the repository root\n");
        return 1;
    }

    printf("\n%zu/%zu passed\n", total - failed, total);
    return failed == 0 ? 0 : 1;
}
