# Developing nvi

- [Testing](#testing)
- [Fuzzing](#fuzzing)

See [BUILD.md](BUILD.md) for compiler requirements and how to build the binary.

## Testing

This project uses [Unity](https://github.com/ThrowTheSwitch/Unity) in combination with [nob.h](https://github.com/tsoding/nob.h).

Build nob (if you haven't already):
```sh
# POSIX
clang -o nob nob.c

# PowerShell
cl nob nob.c
```

Then run a suite (`./nob.exe` on PowerShell):

| Command | Runs |
| --- | --- |
| `./nob unit` | All unit tests |
| `./nob integration` | All integration tests |
| `./nob test` | All test suites |

## Fuzzing

This project uses [libFuzzer](https://llvm.org/docs/LibFuzzer.html) with AddressSanitizer and UndefinedBehaviorSanitizer to fuzz the four untrusted-input surfaces:

| Target    | Harness                     | What it fuzzes                                                                                     |
| --------- | --------------------------- | --------------------------------------------------------------------------------------------------- |
| `parser`  | `tests/fuzz/fuzz_parser.c`  | Arbitrary bytes through `generate_tokens` and `run_parser` as the contents of a single `.env` file.  |
| `matcher` | `tests/fuzz/fuzz_matcher.c` | Arbitrary bytes through `scan_file_content`; the first input byte selects the language accessor set. |
| `args`    | `tests/fuzz/fuzz_args.c`    | NUL-delimited argv entries through `parse_args`.                                                     |
| `config`  | `tests/fuzz/fuzz_config.c`  | Arbitrary bytes through `tokenize_config_file` as the contents of a `.nvi` config file.               |

Fuzzing is POSIX only (Linux, macOS) and requires a clang that ships the libFuzzer runtime.

### Fuzzing requirements

Linux:
- [Clang](https://clang.llvm.org/) (the distro package includes libFuzzer)

macOS:
- [Homebrew LLVM](https://formulae.brew.sh/formula/llvm):

```sh
brew install llvm
```

> [!NOTE]
> Apple's Command Line Tools clang ships ASan and UBSan but not the libFuzzer runtime (`libclang_rt.fuzzer_osx.a`). `./nob fuzz` automatically prefers Homebrew LLVM's clang when present (`/opt/homebrew/opt/llvm` or `/usr/local/opt/llvm`); no `PATH` changes are needed. musl builds (`NVI_LIBC=musl`) and MSVC are not supported.

### Running a fuzz target

Build nob (if you haven't already):
```sh
clang -o nob nob.c
```

Build and run a fuzz target (ctrl-c to stop):
```sh
# defaults to the parser target
./nob fuzz

# or select one explicitly
./nob fuzz parser
./nob fuzz matcher
./nob fuzz args
./nob fuzz config
```

Each target keeps its own cumulative corpus under `build/fuzz/`; interesting inputs found in one run carry over to the next. The parser corpus is seeded from `fixtures/*.env` on first run. When a matching dictionary exists under `tests/fuzz/` (`env.dict`, `matcher.dict`, `args.dict`, `config.dict`), it is passed to libFuzzer automatically to seed the mutator with grammar tokens.

Extra arguments are forwarded to libFuzzer:

```sh
# bounded run (roughly 20s at ~50k exec/s)
./nob fuzz parser -runs=1000000

# reproduce a crash or stall artifact
./nob fuzz parser crash-<hash>
```

### Running all targets

`all` runs every target sequentially with the same forwarded arguments and reports a suite-style summary:

```sh
# regression only: replay every corpus without mutating (useful in CI)
./nob fuzz all -runs=0

# bounded soak of everything, 10 minutes per target
./nob fuzz all -max_total_time=600
```

> [!NOTE]
> `all` requires a `-runs=<N>` or `-max_total_time=<seconds>` bound; an unbounded run would fuzz the first target forever and never reach the rest. Omitting the bound is a usage error.

### Progress output

A watchdog thread (`tests/fuzz/fuzz_watchdog.h`, shared by all harnesses) prints a heartbeat so the fuzzer never appears stuck:

```
[fuzz] alive: execs=256505 (34354/s) elapsed=8s current_input=654 bytes
```

If a single input runs past the stall limit, the watchdog writes it to `fuzz-stall-<pid>.bin` and aborts, turning a hang into a reproducible artifact. libFuzzer's own `-timeout=15` acts as a backstop.

### Fuzzing environment variables

| Variable                 | Description                                                                         |
| ------------------------ | ----------------------------------------------------------------------------------- |
| `FUZZ_HEARTBEAT_SECONDS` | Seconds between heartbeat lines (default: `5`).                                     |
| `FUZZ_STALL_SECONDS`     | Per-input runtime limit before abort + dump (default: `10`).                        |
| `FUZZ_VERBOSE`           | If set, keeps the target's stdout/stderr output (silenced otherwise).               |
| `FUZZ_CC`                | Overrides the compiler used to build the harness.                                   |
| `FUZZ_SAN`               | Overrides the sanitizer list (default: `fuzzer,address,undefined`).                 |

Example, reproducing a single artifact with full diagnostics:

```sh
FUZZ_VERBOSE=1 ./build/fuzz/fuzz_parser fuzz-stall-<pid>.bin
```

> [!NOTE]
> Crash artifacts (`crash-*`, `timeout-*`, `oom-*`, and so on) and stall dumps (`fuzz-stall-*.bin`) are written to the repository root and are gitignored. If the fuzzer finds something, keep the artifact until it's fixed; it is the reproducer.
