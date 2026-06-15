# nvi

A fast, cross-platform, exec-free, RegEx-free `.env` parser, scanner and emitter.

- 0 dependencies
- language agnostic
- parses one or more `.env` files
- handles `${KEY}` interpolations
- supports multiline values via `\` (backslash-newline) delimiter
- can scan project files for environment-variable references across many [languages](https://github.com/mattcarlotta/nvi-bin#supported-file-extensions) and mark them as required
- can validate required keys before command execution

## Installation

Requires [Zig](https://ziglang.org/download/) `0.16.0` or later (optionally install [ZLS](https://zigtools.org/zls/install/) for linting).

```sh
cd ~/Documents

git clone git@github.com:mattcarlotta/nvi-bin.git && cd nvi-bin

# build for production & install somewhere on your $PATH
zig build --release=small --prefix ~/.local

```

For testing/debugging:
```sh
# debug build (default): zig-out/bin/nvi
zig build

# run all test suites
zig build test --summary all

```

### Verify system installation

Run:
```sh
which nvi
# ~/.local/bin/nvi

```
If not found, then `~/.local/bin` probably isn't included in your `$PATH`:
```sh
echo $PATH
```

You'll need to append the `$HOME/.local/bin` path to your shell profile's `$PATH` (eg. `~/.bashrc` or `~/.zshrc`):
```sh
export PATH="$HOME/.local/bin:$PATH"
```

Then source the profile path:
```sh
source <profile_path>
```

## Running

### Unix (Linux, macOS, WSL)

`nvi` emits NUL-delimited ENVs: each `KEY=value` pair, then each command token. `xargs -0` splits the ENVs and hands them to `env`, which sets the variables and runs the command.

```sh
nvi --files .env -- <command> | xargs -0 env
```

> [!NOTE]
> Windows builds will default to the powershell format. Therefore, if using WSL, you'll need to override the format with `--format nul`.

For Windows Non-WSL users, view the [PowerShell Usage](https://github.com/mattcarlotta/nvi-bin#powershell-usage)

More examples:

```sh
# multiple files; later files override earlier ones
nvi --files .env .env.local -- npm start | xargs -0 env

# require keys to be present
nvi --required API_KEY DATABASE_URL -- cargo run | xargs -0 env

# require every env key referenced in source files to be present
nvi --scan mjs --ignored NODE_ENV --files .env -- npm run dev | xargs -0 -r env

# print a single resolved variable
nvi -- printenv MESSAGE | xargs -0 env

# inspect the full child environment
nvi -- env | xargs -0 env

# shell expansion inside the command (single-quote so your shell doesn't expand first)
nvi -- sh -c 'echo "$MESSAGE"' | xargs -0 env

# debug what was parsed (stderr only)
nvi --debug -- echo $?
```
For day-to-day use, you may want to add a function to your shell profile (eg. `~/.zshrc`, `~/.bashrc`):

```sh
nvix() { nvi "$@" | xargs -0 -r env; }
```

NUL delimiting means values pass through byte-exact without quoting or escaping: spaces, quotes, `$`, and even embedded newlines (multiline values) survive intact.

## Flags

| Flag | Alias | Parameters | Description |
| --- | --- | --- | --- |
| `--files` | `-f` | one or more paths | `.env` files to parse, in order. Later files override earlier ones. Paths must be relative to the current directory, must not escape it, and must contain `.env`. Defaults to `.env`. |
| `--ignored` | `-i` | one or more keys | Keys that `scan` should not add to the required ENV set (e.g. `NODE_ENV`, which is typically injected at runtime by tooling rather than defined in a `.env` file). Only meaningful together with `scan`. |
| `--required` | `-r` | one or more keys | Keys that must exist with non-empty values after parsing. `nvi` exits with an error listing any that are missing. |
| `--format` | `-F` | `nul` or `powershell` | Output format. Defaults to `nul` on Unix and `powershell` on Windows (chosen at compile time per target). |
| `--debug` | `-d` | | Print parsed flags, tokens, scan results, and the resolved env listing to stderr. The env listing follows the active `--format`, so it previews the exact pair syntax that will hit stdout. |
| `--help` | `-h` | | Prints usage help and exits. |
| `--scan` | `-s`| one or more file extensions | Recursively scans `*.<ext>` files for environment-variable accessors (e.g. `process.env.KEY`, `os.getenv("KEY")`) and marks the referenced keys as required.† |
| `--version` | `-v` | | Prints versions and exits. |
| `--` | | command tokens | End-of-options delimiter. Everything after it is passed through to stdout untouched, as the command for the downstream consumer to run. Required (except for standalone `scan`, `help`, and `version`). |

Unrecognized flags (and their parameters) are warned about on stderr and ignored.

## Commands

| Command | Parameters | Description |
| --- | --- | --- |
| `scan` | one or more file extensions | Recursively scans `*.<ext>` files for environment-variable accessors (e.g. `process.env.KEY`, `os.getenv("KEY")`) and marks the referenced keys as required.† |
| `help` | | Prints usage help. |
| `version` | | Prints version. |

> [!NOTE]
> † Without a `--` command, scan reports what it finds and exits. With a `--` command, scan sets the found ENV keys to the required ENV set.

Unrecognized commands are warned about on stderr and ignored.

### Exit codes

- `0` - Ok: Parsed/emitted ENVs successfully or prints information and exits (help, scan, version)
- `1` - Operational failure: out of memory, file unreadable, parse error, missing required keys, or output write failure
- `2` - Usage error: invalid flags or missing `--` command

The exit code of *your command* is reported by the downstream consumer (`xargs`), not by `nvi`.

## Scanning for ENV keys

`-s`, `--scan` or just `scan` followed by one or many file `ext`, walks a project's file tree from the current directory and, for each file matching the given extensions, looks for the environment-variable accessors of that file's language.

For example, every line below is recognized and yields the key `DATABASE_URL`:

```
process.env.DATABASE_URL          # JavaScript / TypeScript
process.env["DATABASE_URL"]
import.meta.env.DATABASE_URL      # Vite
os.getenv("DATABASE_URL")         # Python
os.Getenv("DATABASE_URL")         # Go
env::var("DATABASE_URL")          # Rust
ENV["DATABASE_URL"]               # Ruby
System.getenv("DATABASE_URL")     # Java / Kotlin
$env:DATABASE_URL                 # PowerShell
$ENV{DATABASE_URL}                # Perl
```
> [!IMPORTANT]
> An environment-variable will be detected by *how it's accessed* and not by how it's spelled (indepedent of its casing, prefix, or suffix). That said, ideally, ENVs should be UPPERCASE_SNAKE_CASE.

> [!CAUTION]
> Dynamic keys (`process.env[name]`), destructured variables (`const { DATABASE_URL } = process.env`), and aliased accessors (`const e = process.env; e.DATABASE_URL`) cannot be detected by the scanner without a per-language AST (which this tool avoids) and therefore won't be reported.

### Supported file extensions:
- C -> `c`
- Clojure -> `clj`, `cljs`, `cljc`
- Crystal -> `cr`
- C++ -> `cc`, `cpp`, `cxx`, `h`, `hh`, `hpp`, `hxx`
- C# -> `cs`
- D -> `d`
- Dart -> `dart`
- Elixir -> `ex`, `exs`
- Erlang -> `erl`, `hrl`
- Fortran -> `f`, `f90`, `f95`, `f03`, `f08`, `for`
- F# -> `fs`, `fsi`, `fsx`
- Go -> `go`
- Groovy -> `groovy`
- Haskell -> `hs`, `lhs`
- Java -> `java`, `gradle`
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
- Python -> `py`, `pyi`
- R -> `r`
- Ruby -> `gemspec`, `rb`, `rake`
- Rust -> `rs`
- Scala -> `sc`, `scala`
- Swift -> `swift`
- Tcl -> `tcl`
- V -> `v`
- Visual Basic -> `vb`
- Zig -> `zig`

Examples:

```sh
# report every env reference in .mjs and .ts files, then exit
nvi --scan mjs ts

# scanned keys are required and must be defined before 'npm run dev' is emitted
nvi --scan mjs --files .env -- npm run dev | xargs -0 -r env

# exclude runtime-injected keys from the command environment
nvi --scan mjs --ignored NODE_ENV --files .env -- npm run dev | xargs -0 -r env
```

Notes:

- Extensions may be written as `ext`, `.ext`, or `'*.ext'` (see tip below).
- Extensions with no known accessor patterns are skipped. Shell scripts are intentionally not scanned, because `$VAR` is indistinguishable from any non-environment shell variable.
- Dot-directories (eg. `.git`, `.next`, `.venv`, and so on) and common dependency/cache/build-output directories (eg. `node_modules`, `__pycache__`, `zig-out`, and so on) are ignored (see [blacklist](https://github.com/mattcarlotta/nvi-bin/blob/main/src/scanner.zig#L12-L35)). Symlinked directories are not followed.
- When a command is present, missing scanned keys exit with code `1`, the same as `--required` failures, and the command is never emitted.

> [!TIP]
> When using `*.ext` globs, use quotes (`'*.ext'`) or just pass bare extensions; unquoted globs are expanded into filenames by your shell before `nvi` runs.

## `.env` file syntax

Here are some examples of how ENVs can be defined an `.env` file:

```dotenv
# comments start with a hash (inline comments not supported)

# a literal value
MESSAGE=hello

# an interpolation ${KEY} represents a value from either the shell
# environment or from a KEY in an .env file (must be defined before use)
# for example: ${MESSSAGE} world -> hello world
GREETING=${MESSAGE} world

# an equal sign after a key is a literal '=' (not a nested key)
BASE64_OK=abc==

# a dollar sign without braces is a literal '$' (not an interpolated key)
PRICE=$5.00

# backslash-newline continues the value (interpolation keys still work)
SSH_PRIVATE_KEY=-----BEGIN RSA PRIVATE KEY-----\
MIIEpAIBAAKCAQEA2x5s8K9vN3pQ7mK8vL2d5pJ9mX6kL8qR3wT9uV5sZ2aB4cD\
oqRosTouVoaV1EthzxeIRx7pPoqR9sTiuVcwXjyZiBvcDj0FlgHgiJjlLjmNjoP\
owKBAQDZ2sX7pPoqRisTiuVcwXjyZiBvcDj0FlgHgiJjlLjmNjoPoqRosTouVoaV\
3EthzxeIRx7pPoqR9sTiuVcwXjyZiBvcDj0FlgHgiJjlLjmNjoPoqRosTouVoaV\
-----END RSA PRIVATE KEY-----
# no backslash with just a new-line/EOF indicates the end of a multi-line value

```

Interpolated keys resolve first from the shell environment and then from keys parsed earlier (including earlier `--files`).
Undefined interpolations and keys with empty values are skipped with a warning.


### PowerShell Usage

The Windows build defaults to `--format powershell`, emitting `$env:` assignments followed by a call-operator invocation.
PowerShell evaluates the emitted script: `Out-String` joins nvi's output back into a single string (PowerShell splits native stdout into lines, which would break multiline values), and `Invoke-Expression` executes it.

```powershell
nvi --files .env -- npm run dev | Out-String | Invoke-Expression
```

For day-to-day use, you may want to add a function to your PowerShell `$PROFILE`:

```powershell
function nvix { nvi @args | Out-String | Invoke-Expression }
```

then usage:

```powershell
nvix --files .env -- <command>
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
- **Git Bash / MSYS2:** if you have GNU `xargs` and `env` available, the native Windows binary can use the Unix pipeline directly with `--format nul`.
- **WSL:** use the Linux binary and the Unix pipeline.
- `cmd.exe` is not supported.

### Choosing a format explicitly

`--format` overrides the platform default in either direction:

```sh
# preview PowerShell ENV format
nvi --format powershell --debug
# nvi --format powershell -- echo $?

# preview Unix ENV format
nvi --format nul --debug
# nvi --format nul -- echo $?
```

## Security model

`nvi` doesn't perform execution operations (like `exec`/`syscall`), no process spawning nor shell invocation. It reads the `.env` files you provide and writes them as bytes to stdout.
Process execution happens entirely in the downstream consumer you choose (`xargs`/`env` or PowerShell), with the command tokens you've typed.
For PowerShell, values are emitted inside single-quoted strings (the only escape being `''`), so values cannot break out of string context into executable position.

### [Contributing](https://github.com/mattcarlotta/nvi-bin/blob/main/CONTRIBUTING.MD)
### [License](https://github.com/mattcarlotta/nvi-bin/blob/main/LICENSE.md)
