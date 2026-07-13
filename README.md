# nvi

A fast and minimal cross-platform CLI `.env` parser, environment-variable scanner and emitter.

- 0 dependency
- Language and framework agnostic (replaces language specfic env packages)
- Sequentially parses one or more `.env` files
- Supports `${KEY}` interpolations, `#` comments, `'` and `"` quotes, and `\` delimited multiline values
- Scans project files for environment-variable references across many [languages](#supported-file-extensions-to-the-right-of-the-language) and marks them as required
- Checks required environment-variables are defined before command execution
- Supports ignoring environment-variables that may be set at run-time
- Loads flags from a [`.nvi` config file](#nvi-config-file)

## Installation

For the best compatibility [build and install from source](#build-and-install-from-source).

Otherwise, download a precompiled binary from [releases](https://github.com/mattcarlotta/nvi/releases/).

Then extract the precompiled binary and place it within a directory recognized by `$PATH` (POSIX) or `Path` (PowerShell).

If you're not sure if the destination directory is recognized by your shell, use the corresponding links below and skip to the instructions about path recognition:
- [POSIX](#posix-linux-macos-wsl)
- [PowerShell](#powershell-windows)

## Build and install from source

Optional requirements:
- [Clangd](https://clangd.llvm.org/)
- [Clang Format](https://clang.llvm.org/docs/ClangFormat.html)

Building source code:
- [nob.h](https://github.com/tsoding/nob.h)

### POSIX (Linux, macOS, WSL)

Requirements:
- [Clang](https://clang.llvm.org/) (default), or [GCC](https://gcc.gnu.org/) via `NVI_CC=gcc`
- [LLD](https://lld.llvm.org/) on Linux when building with clang (release builds link with `-fuse-ld=lld`; usually packaged as `lld`; not needed for gcc builds)

Clone repo and build `nob`:
```sh
cd ~/Downloads

git clone git@github.com:mattcarlotta/nvi.git && cd nvi

clang nob.c
```

Build for debugging (not required):
```sh
./nob
```

Build for release (not required):
```sh
./nob release
```

Before placing or installing a release binary into a directory, ensure the destination directory is recognized as a shell `$PATH` owned by `$USER` and not by `root`:
```sh
echo $PATH | tr ':' '\n' | xargs -I{} sh -c 'printf "%-50s %s\n" "{}" "$(stat -f "%Su:%Sg" "{}" 2>/dev/null)"' | nl
```

If there aren't any `$USER` own bin directories, then create a local bin directory:
```sh
mkdir -p $HOME/.local/bin
```

Then edit and update your shell profile's (eg. `~/.bashrc` or `~/.zshrc`) `$PATH` to include the following:
```sh
PATH+=("$HOME/.local/bin")
```

Then source (reload) the profile (eg. `~/.bashrc` or `~/.zshrc`):
```sh
source <PROFILE>
```

Lastly, build and install the release binary into the destination `<DIR>`:
```sh
# install in a directory that is recognized by the shell $PATH
# for example, if you followed the instruction sabove, then: ./nob install $HOME/.local/bin
./nob install <DIR>
```

> [!NOTE]
> To build a fully static, portable Linux binary with musl instead of clang+glibc, install `musl-tools` and run `NVI_LIBC=musl ./nob <release|install>`.

> [!NOTE]
> To build with GCC instead of clang, run `NVI_CC=gcc ./nob <cmd>` (any GCC 11+ works; a versioned name like `NVI_CC=gcc-14` is also fine). `NVI_LIBC=musl` takes precedence over `NVI_CC`. GCC release builds use a conservative flag set (no `-flto`/lld pipeline), so clang remains the recommended compiler for the smallest release binaries. Fuzzing always requires clang.

To verify system installation, run:
```sh
which nvi
# <DIR>

nvi version
# nvi <version> (<build_type>)
# commit <commit>
# clang|gcc <version>
# <architecture>
```

### PowerShell (Windows)

Requirements:
- [MSVC](https://visualstudio.microsoft.com/vs/features/cplusplus/)
- [Clang for MSVC](https://clang.llvm.org/get_started.html#buildWindows)

Follow these steps:
1. Install MSVC Build Tools:
```powershell
winget install Microsoft.VisualStudio.2022.BuildTools --source winget
```

> [!NOTE]
> It should open a GUI installer, where you need to select and install the `Desktop development with C++` workload. This gives you the MSVC linker, Windows SDK, and CRT libraries.

2. Install LLVM/Clang:
```powershell
winget install LLVM.LLVM --source winget
```

3. Add clang to `Path`:
```powershell
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\Program Files\LLVM\bin", "User")
```

4. Close and reopen PowerShell

5. Launch a developer shell:
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64
```

> [!CAUTION]
> Spawning a VS Dev Shell without the `-Arch` and `-HostArch` flags may result in a 32bit (instead of 64bit) shell environment.

6. Change directory to `Documents`:
```powershell
cd Documents
```

7. Clone repo (assumes `git` is installed and you have registered your SSH key to your Github account):
```powershell
git clone git@github.com:mattcarlotta/nvi.git
```
Optionally download it:
```powershell
Invoke-WebRequest -Uri "https://github.com/mattcarlotta/nvi/archive/refs/heads/main.zip" -OutFile "nvi.zip"
```
Then extract it:
```powershell
Expand-Archive -Path "nvi.zip" -DestinationPath "nvi"
```
Then set up git tracking (the git commit will be used within the output for `nvi version`; otherwise, it'll just report the commit as "unknown"):
```powershell
git init
git remote add origin https://github.com/mattcarlotta/nvi.git
git fetch origin
git reset origin
```

8. Change directory to `nvi`:
```powershell
cd nvi
```

9. Build `nob.c`:
```powershell
cl nob.c
```

Build for debugging (not required):
```powershell
./nob.exe
```

Build for release (not required):
```powershell
./nob.exe release
```

Before placing or installing a release binary in a directory, ensure the destination directory is recognized as a PowerShell `Path`:
```powershell
$env:Path -split ';'
```

If not, then add the destination directory `<DIR>` to the PowerShell `Path` (swap `<DIR>` below for the destination; eg, `C:\tools\bin`):
```powershell
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";<DIR>", "User")
```

Build and install a release binary to the destination directory (change `<DIR>` to the destination directory):
```powershell
./nob.exe install <DIR>
```

To verify system installation, close and reopen PowerShell, then run:
```powershell
Get-Command nvi

nvi version
```

## Running

### Run in POSIX (Linux, macOS, WSL)

The POSIX build defaults to `--format nul`, emitting: `KEY=value\0` assignments followed by shell commands. Then `xargs -0` splits
the emitted ENVs and hands them off to `env`, which sets the ENVs to the shell, and then the command runs.

```sh
nvi [flags|command] -- [command] | xargs -0 env
```
For day-to-day use, you may want to add a function to your shell profile (eg. `~/.zshrc`, `~/.bashrc`):

```sh
nvix() { nvi "$@" | xargs -0 -r env; }
```
Then source (reload) the profile (eg. `~/.bashrc` or `~/.zshrc`):
```sh
source <profile_url>
```
To verify it's available, run:
```sh
which nvix
```
### Run in PowerShell (Windows)

The Windows build defaults to `--format powershell`, emitting `$env:` assignments followed by a call-operator invocation.
PowerShell evaluates the emitted script: `Out-String` joins nvi's output back into a single string and `Invoke-Expression` executes it.

```powershell
nvi [flags|command] -- [command] | Out-String | Invoke-Expression
```

For day-to-day use, you may want to add a function to your PowerShell `$PROFILE`:

```powershell
notepad $PROFILE
```
Then add this function and save:
```powershell
function nvix { nvi @args | Out-String | Invoke-Expression }
```
Close and reopen PowerShell, then run:
```powershell
Get-Command nvix
```

Example of what the emitted structure looks like:

```powershell
$env:MESSAGE = 'hello'
$env:MULTI = 'line1
line2'
& 'npm' 'run' 'dev'
```

Values are single-quoted with PowerShell's one escaping rule (`'` doubled to `''`), so apostrophes, `$`, backticks, and newlines are all literal.

Notes for Windows users:

- **Persistence:** `$env:` assignments apply to the invoking PowerShell session, so the variables remain set after the command exits. For an isolated, throwaway environment, run the pipeline inside `pwsh -Command "..."`.
- **Encoding:** PowerShell decodes nvi's output using the console encoding. PowerShell 7+ defaults to UTF-8; on Windows PowerShell 5.1, set `[Console]::OutputEncoding` to UTF-8 if your values contain non-ASCII characters.
- **Git Bash / MSYS2:** if you have GNU `xargs` and `env` available, the native Windows binary can use the POSIX pipeline directly with `--format nul`.
- **WSL:** use the Linux binary and the POSIX instructions.
- `cmd.exe` is not supported.

## Flags

| Flag Usage | Description |
| --- | --- |
| `-d, --dry-run` | Prints results to stderr and exits with 0. |
| `-f, --files <file> ...`| Parses one or more `.env` files in sequential order. |
| `-F, --format <format>` | Formats ENVs for the downstream consumer (formats: `nul` or `powershell`). |
| `-h, --help` | Prints usage help to stdout and exits with 0. |
| `-i, --ignored <KEY> ...` | Ignores a list of keys that a `scan` may add to the required ENV list. |
| `-r, --required <KEY> ...` | Requires a list of keys that must be defined after parsing. |
| `-R, --reveal` | Reveals ENV values in a dry-run; otherwise, they'll be hidden (`*****`). |
| `-s, --scan <ext> ...` | Recursively scans [`<ext>`](#supported-file-extensions-to-the-right-of-the-language) files for environment-variable accessors. † |
| `-t, --threads <1-255>` | Number of threads to use when scanning files (max: CPU core count). †† |
| `-v, --version` |  Prints version info to stdout and exits with 0. |
| `@<config>` | Loads flags from a [`.nvi` config file](#nvi-config-file) (eg. `@.nvi.development`). |
| `--` <command> | An end-of-options delimiter followed by a `<command>` (eg. `npm run dev`). |

> † without a `--` command, scan will only report what it finds and exit (must include **--dry-run**); with a `--` command, scan sets the found ENV keys to the required ENVs list.

> †† using more threads than available CPU cores and/or the OS's file IO limitations will degrade scanning performance

Unrecognized flags or arguments are usage errors.

Diagnostics written to stderr are colorized only when stderr is a TTY; however, setting a non-empty `NO_COLOR` env disables the color:
```sh
NO_COLOR=true nvi [flags]
```

## Usage examples

```sh
# multiple files; later files override earlier ones
nvi --files .env .env.local -- npm start | <consumer>

# require keys to be present
nvi --files .env --required API_KEY DATABASE_URL -- cargo run | <consumer>

# saves a dry run log of what was scanned, tokenized, and parsed
nvi --files .env --scan ts --dry-run >2 nvi.log; less nvi.log

# require every env key referenced in py source files to be present
nvi --files .env --scan py -- python main.py  | <consumer>

# POSIX shell expansion inside the command (single-quote so your shell doesn't expand first)
nvi --files .env -- sh -c 'echo "$MESSAGE"' | <consumer>
```

### Exit codes

- `0` - Success: emits ENVs for a downstream consumer or prints information and exits (help, scan, version)
- `1` - Operational failures: out of memory, file unreadable, parser errors, or required keys are undefined
- `2` - Usage errors: flags missing required params, invalid flags/params, or a missing `--` command

The exit code of *your command* will be reported by the downstream consumer, not by `nvi`.

## `.nvi` config file

Just like `.env` files, you may use one or many `.nvi` config files to load project and/or environment specific flags.

Usage:
```sh
nvi @<path> -- <command> | <consumer>
```

Example config:
```sh
# .nvi.local
--files .env .env.local
--format nul
--scan ts tsx mjs
--ignored NODE_ENV CI
--threads 4
```

You'll still have the option to append or override flags after a config file (except for flags that don't have parameters, like: `--dry-run`):
```sh
# the .nvi.local config (above) supplies the defaults, but the flags
# specified afterward append .env.production to files and override the format
nvi @.nvi.local --files .env.production -F powershell -- <command> | <consumer>
```

Rules:
- Only loads a single `.nvi` file (referencing other `.nvi` configs is unsupported).
- Flags and parameters must be defined on the same line.
- A `--` command is not allowed inside a config file; commands stay within the command line, where it'll be handled by the downstream consumer.
- An empty or comment-only config file is an error (matching the empty `.env` file behavior).

## Scanning for ENV keys

`-s`, `--scan` followed by one or many file `ext`, walks a project's file tree from the current directory and, for each file matching the given extensions, looks for the environment-variable accessors of that file's language.

For example, every line below would be recognized and yields the key `DATABASE_URL`:

```
process.env.DATABASE_URL          # JavaScript / TypeScript
process.env["DATABASE_URL"]
import.meta.env.DATABASE_URL
os.getenv("DATABASE_URL")         # Python
os.Getenv("DATABASE_URL")         # Go
env::var("DATABASE_URL")          # Rust
ENV["DATABASE_URL"]               # Ruby
System.getenv("DATABASE_URL")     # Java / Kotlin
$ENV{DATABASE_URL}                # Perl
```
> [!IMPORTANT]
> An environment-variable will be detected by *how it's accessed* and not by how it's spelled (indepedent of its casing, prefix, or suffix). That said, ideally, ENVs should be UPPER_CASE_SNAKE_CASE.

> [!CAUTION]
> The following will not be detected by the scanner...

- Dynamic keys:
```js
const key = "DATABASE_URL";
process.env[key];
```
- Destructured variables:
```js
const { DATABASE_URL } = process.env;
```
- Aliased accessors:
```js
const e = process.env;
e.DATABASE_URL;
```

### Supported file extensions (to the right of the language)
- C -> `c`, `h`
- Clojure -> `clj`, `cljs`, `cljc`
- Crystal -> `cr`
- C++ -> `cc`, `cpp`, `cxx`, `hh`, `hpp`, `hxx`
- C# -> `cs`
- D -> `d`
- Dart -> `dart`
- Elixir -> `ex`, `exs`
- Erlang -> `erl`, `hrl`
- Fortran -> `f`, `f90`, `f95`, `f03`, `f08`, `for`
- F# -> `fs`, `fsi`, `fsx`
- Go -> `go`
- Gradle -> `gradle`
- Groovy -> `groovy`
- Haskell -> `hs`, `lhs`
- Java -> `java`
- JavaScript/TypeScript -> `cjs`, `cts`, `js`, `jsx`, `mjs`, `mts`, `ts`, `tsx`
- Julia -> `jl`
- Kotlin -> `kt`, `kts`
- Lua -> `lua`
- Nim -> `nim`
- Nushell -> `nu`
- Objective-C -> `m`, `mm`
- OCaml -> `ml`, `mli`
- Pascal/Delphi -> `dpr`, `pas`, `pp`
- Perl -> `pl`, `pm`, `t`
- PHP -> `php`
- PowerShell -> `ps1`, `psm1`, `psd1`
- Python -> `py`, `pyi`, `pyw`
- R -> `r`
- Ruby -> `gemspec`, `rb`, `rake`
- Rust -> `rs`
- Scala -> `sc`, `scala`
- Swift -> `swift`
- Tcl -> `tcl`
- V -> `v`
- Visual Basic -> `vb`
- YAML -> `yaml`, `yml` †
- Zig -> `zig`

> † YAML has no language-level accessor, so the scanner matches POSIX-style parameter expansion: `${KEY}` plus the operator forms `${KEY:-default}`, `${KEY:?err}`, etc.
> A bare `$KEY`, `$${KEY}`, and `${{ ... }}` expressions are ignored.

#### Scan Usage Examples

```sh
# scans for matching ENVs within .mjs and .ts files using 4 threads, reports findings, then exits
nvi --scan mjs ts --threads 4 --dry-run

# collects scanned keys to be required and defined before the 'node index.js' command is emitted
nvi --scan mjs --files .env -- node index.mjs | <consumer>

# ignores runtime-injected ENVs often found within 'npm run dev' (node) environment
nvi --scan mjs --ignored NODE_ENV --files .env -- npm run dev | <consumer>
```

> [!IMPORTANT]
> There may be a bottleneck with how many threads can be used at one time to scan files. A general rule of thumb is to start with 4 threads (if available) and then increase by 2.
> For example, if a CPU has 16 cores, start with 4 threads, then 6, then 8... up to the max CPU core count (16 threads).
> More is not always better! See Threaded Scan Results below...

<details>
<summary>Threaded Scan Results</summary>
Warm cached and scanning the same large codebase...

MacBook Pro M4 Max running Mac OS Tahoe 26.5.2:
| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `nvi --scan ts tsx mjs cjs js jsx rs --threads 1 --dry-run` | 584.0 ± 2.5 | 580.7 | 589.9 | 2.49 ± 0.26 |
| `nvi --scan ts tsx mjs cjs js jsx rs --threads 4 --dry-run` | 234.2 ± 24.1 | 215.7 | 298.7 | 1.00 |
| `nvi --scan ts tsx mjs cjs js jsx rs --threads 6 --dry-run` | 259.5 ± 13.1 | 241.0 | 328.4 | 1.11 ± 0.13 |
| `nvi --scan ts tsx mjs cjs js jsx rs --threads 8 --dry-run` | 338.2 ± 7.1 | 318.9 | 352.7 | 1.44 ± 0.15 |
| `nvi --scan ts tsx mjs cjs js jsx rs --threads 16 --dry-run` | 736.4 ± 26.7 | 655.1 | 774.5 | 3.14 ± 0.34 |

Custom Desktop AMD 5950x running Linux Mint 21.2:
| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `../nvi-bin/nvi --scan ts tsx mjs cjs js jsx rs --threads 1 --dry-run` | 323.4 ± 7.0 | 310.3 | 341.9 | 8.78 ± 0.76 |
| `../nvi-bin/nvi --scan ts tsx mjs cjs js jsx rs --threads 4 --dry-run` | 101.3 ± 4.6 | 84.7 | 112.0 | 2.75 ± 0.26 |
| `../nvi-bin/nvi --scan ts tsx mjs cjs js jsx rs --threads 6 --dry-run` | 72.6 ± 3.3 | 66.0 | 78.7 | 1.97 ± 0.19 |
| `../nvi-bin/nvi --scan ts tsx mjs cjs js jsx rs --threads 8 --dry-run` | 60.4 ± 3.4 | 51.0 | 65.9 | 1.64 ± 0.17 |
| `../nvi-bin/nvi --scan ts tsx mjs cjs js jsx rs --threads 16 --dry-run` | 44.5 ± 5.0 | 33.1 | 52.8 | 1.21 ± 0.17 |
| `../nvi-bin/nvi --scan ts tsx mjs cjs js jsx rs --threads 32 --dry-run` | 36.9 ± 3.1 | 26.4 | 40.7 | 1.00 |

The test numbers above **ARE NOT** meant to be a measurement nor a comparison for how fast the scanner can run on a given system, but instead to showcase how a system can have file IO limitations past a certain number of threads. For the MacBook Pro, more threads degraded the scanning performance, whereas for the custom desktop it was logarithmically better.
</details>

Notes:

- Extensions must be written as `ext` and not `.ext` or `*.ext`.
- Extensions with no known accessor patterns are usage errors.
- Dot-directories (eg. `.git`, `.next`, `.venv`, and so on) and common dependency/cache/build-output directories (eg. `node_modules`, `__pycache__`, `zig-out`, and so on) are ignored.
- Symlinked directories are not followed.

## `.env` file syntax

Here are some examples of how ENVs can be defined in an `.env` file:

```dotenv
# comments start with a hash (inline comments not supported)

# a literal value
MESSAGE=hello

# an interpolation ${KEY} can be used after a key and it represents a value
# from either the shell environment or from a KEY in an .env file (must be defined before use)
# for example: ${MESSSAGE} world => hello world
GREETING=${MESSAGE} world

# an equal sign after a key is a literal '=' (not a nested key)
BASE64_OK=abc==

# a dollar sign after a key without braces is a literal '$'
PRICE=$5.00

# a hash sign after a key is a literal '#' (not a comment)
CHANNEL=#why-is-this-bug-occuring

# single or double quotes after a key are stripped from the value,
# but the inner whitespace and characters are preserved as is
DOUBLE_QUOTES="     hello world     "

# single quoted values after a key WILL NOT interpolate ${KEY}
SINGLE_QUOTES='abc${NOT_AN_INTERPOLATED_KEY}'

# double quoted values after a key WILL interpolate ${KEY}
GOODBYE="I will never say ${GREETING} ever again"

# single or double qoutes within a value after a key are not stripped
# and are treated as literal characters
MESSAGE=she said "hello world" in death
RESPONSE=then he said 'goodbye my love' in life

# an explicitly empty quoted value after a key is allowed, but a
# bare 'KEY=' without a value is an error
EMPTY_OK=""

# a POSIX shell-style export prefix is stripped
export EXPORTED=value

# a POSIX shell-style default ':-' value after a key is supported
# for an interpolated ${KEY} (it's used when the KEY is unset or empty)
RETRIES=${MAX_RETRIES:-3}

# backslash-newline continues a multiline value
# an interpolation ${KEY} will still work on any same line
SSH_PRIVATE_KEY=-----BEGIN RSA PRIVATE KEY-----\
MIIEpAIBAAKCAQEA2x5s8K9vN3pQ7mK8vL2d5pJ9mX6kL8qR3wT9uV5sZ2aB4cD\
oqRosTouVoaV1EthzxeIRx7pPoqR9sTiuVcwXjyZiBvcDj0FlgHgiJjlLjmNjoP\
owKBAQDZ2sX7pPoqRisTiuVcwXjyZiBvcDj0FlgHgiJjlLjmNjoPoqRosTouVoaV\
3EthzxeIRx7pPoqR9sTiuVcwXjyZiBvcDj0FlgHgiJjlLjmNjoPoqRosTouVoaV\
-----END RSA PRIVATE KEY-----
# when there's no backslash and just a new-line or EOF, then that
# indicates the end of a multiline value
```

- Keys must match `[A-Za-z_][A-Za-z0-9_]*`; anything else is a tokenizer error.
- Interpolated keys resolve first from the shell environment and then from any keys parsed from earlier `.env` files specified by `--files`.
- An undefined key interpolation without a `:-` fallback, a bare `KEY=` with no value, or a `--required` key that is undefined/empty after parsing are parser errors.

## Testing

This project uses [Unity](https://github.com/ThrowTheSwitch/Unity) in combination with [nob.h](https://github.com/tsoding/nob.h)

### POSIX

Build nob (if you haven't already):
```sh
clang -o nob nob.c
```

Run all unit tests:
```sh
./nob unit
```

Run all integration tests:
```sh
./nob integration

```

Run all test suites:
```sh
./nob test
```

### PowerShell

Build nob (if you haven't already):
```powershell
cl nob nob.c
```

Run all unit tests:
```powershell
./nob.exe unit
```

Run all integration tests:
```powershell
./nob.exe integration
```

Run all test suites:
```powershell
./nob.exe test
```

## Fuzzing

This project uses [libFuzzer](https://llvm.org/docs/LibFuzzer.html) with AddressSanitizer and UndefinedBehaviorSanitizer to fuzz the four untrusted-input surfaces:

| Target    | Harness                     | What it fuzzes                                                                                     |
| --------- | --------------------------- | --------------------------------------------------------------------------------------------------- |
| `parser`  | `tests/fuzz/fuzz_parser.c`  | Arbitrary bytes through `generate_tokens` and `run_parser` as the contents of a single `.env` file.  |
| `matcher` | `tests/fuzz/fuzz_matcher.c` | Arbitrary bytes through `scan_file_content`; the first input byte selects the language accessor set. |
| `args`    | `tests/fuzz/fuzz_args.c`    | NUL-delimited argv entries through `parse_args`.                                                     |
| `config`  | `tests/fuzz/fuzz_config.c`  | Arbitrary bytes through `tokenize_config_file` as the contents of a `.nvi` config file.               |

Fuzzing is POSIX only (Linux, macOS) and requires a clang that ships the libFuzzer runtime.

### Requirements

Linux:

- [Clang](https://clang.llvm.org/) (the distro package includes libFuzzer)

macOS:

- [Homebrew LLVM](https://formulae.brew.sh/formula/llvm):

```sh
brew install llvm
```

> [!NOTE]
> Apple's Command Line Tools clang ships ASan and UBSan but not the libFuzzer runtime (`libclang_rt.fuzzer_osx.a`). `./nob fuzz` automatically prefers Homebrew LLVM's clang when present (`/opt/homebrew/opt/llvm` or `/usr/local/opt/llvm`); no `PATH` changes are needed. musl builds (`NVI_LIBC=musl`) and MSVC are not supported.

### Running

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

A watchdog thread (`tests/fuzz/fuzz_watchdog.h`, shared by all harnesses) prints a heartbeat so the fuzzer never looks hung:

```
[fuzz] alive: execs=256505 (34354/s) elapsed=8s current_input=654 bytes (0.0s)
```

If a single input runs past the stall limit, the watchdog writes it to `fuzz-stall-<pid>.bin` and aborts, turning a hang into a reproducible artifact. libFuzzer's own `-timeout=15` acts as a backstop.

### Environment variables

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

## Security model

- It doesn't perform file execution operations (like [exec](https://man7.org/linux/man-pages/man3/exec.3p.html)), nor process spawning nor shell invocation.
- It doesn't use any [regular expressions](https://pubs.opengroup.org/onlinepubs/7908799/xsh/regex.h.html) and never will for as long as I live!
- It will only parse the `.env` files you provide and write parsed ENVs to stdout.
- Process execution happens entirely in the downstream consumer you choose (`xargs`/`env` or PowerShell), with the command tokens you've typed.
- For PowerShell, values are emitted inside single-quoted strings (the only escape being `''`), so values cannot break out of string context into executable position.

### [Contributing](CONTRIBUTING.MD)
### [License](LICENSE.md)
