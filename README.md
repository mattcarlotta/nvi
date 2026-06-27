# nvi

A fast, cross-platform, exec-free, RegEx-free `.env` parser, scanner and emitter.

- Zero dependency
- Language agnostic
- Parses one or more `.env` files
- Handles `${KEY}` interpolations
- Supports multiline values via `\` (backslash-newline) delimiter
- Optionally scans project files for environment-variable references across many [languages](https://github.com/mattcarlotta/nvi-bin#supported-file-extensions) and set them as required
- Optionally validates required keys are defined before command execution

## Installation

Download a precompiled binary from [Releases](https://github.com/mattcarlotta/nvi-bin/releases/)

Then place the binary in one of the directories (owned by `$USER`, not by `root`) located in your shell `$PATH`:
```sh
echo $PATH | tr ':' '\n' | xargs -I{} sh -c 'printf "%-50s %s\n" "{}" "$(stat -f "%Su:%Sg" "{}" 2>/dev/null)"' | nl
```

Optionally, create a self-owned local bin directory:
```sh
mkdir -p $HOME/.local/bin
```

Then edit and update your shell  profile's (eg. `~/.bashrc` or `~/.zshrc`) `$PATH` to include the following:
```sh
typeset -U path # optionally remove duplicate directories in $PATH
path+=("$HOME/.local/bin")
```

Then source (reload) the profile (eg. `~/.bashrc` or `~/.zshrc`):
```sh
source <profile_url>
```

## Build and install from source

### Requirements:

Mac-OS and GNU Linux:
- [Clang](https://clang.llvm.org/) `21.0.0` or later

Windows:
- [Clang for MSVC](https://clang.llvm.org/get_started.html#buildWindows) `21.0.0` or later

Optional:
- [Clangd](https://clangd.llvm.org/) `21.0.0` or later (for LSP)
- [Clang Format](https://clang.llvm.org/docs/ClangFormat.html) `21.0.0`

Building source code:
- [nob](https://github.com/tsoding/nob.h)

Clone project and build `nob`:
```sh
cd ~/Downloads

git clone git@github.com:mattcarlotta/nvi-bin.git && cd nvi-bin

clang -o nob nob.c
```

### Build for debugging

Run `nob` without arguments:
```sh
./nob
```
Then usage becomes:
```
./nvi [flags] -- <command>
```

### Build for release

Run `nob`:
```sh
./nob release
```
Then usage becomes:
```
./nvi [flags] -- <command>
```

### Build and install release

Run nob:
```sh
# install in a directory that is recognized by the shell $PATH
DIR=<directory> ./nob install
```
Then usage becomes:
```
nvi [flags] -- <command>
```

> [!NOTE]
> Windows builds will default to the powershell format. Therefore, if not using [WSL](https://learn.microsoft.com/en-us/windows/wsl/install), you'll need to override the format flag with `--format nul`.

### Verify system installation

Run:
```sh
which nvi
# ~/.local/bin/nvi

nvi version
# nvi <version> (<build_type>)
# commit <commit>
# clang <version>
# <architecture>
```

If not found, then see [Installation](https://github.com/mattcarlotta/nvi-bin#installation) instructions about a missing shell `$PATH`.

## Running

### Unix (Linux, macOS, WSL)

`nvi` emits NUL-delimited ENVs: each `KEY=value` pair, then each command token. `xargs -0` splits the ENVs and hands them to `env`, which sets the variables and runs the command.

```sh
nvi [flags|command] -- [command] | xargs -0 env
```
For day-to-day use, you may want to add a function to your shell profile (eg. `~/.zshrc`, `~/.bashrc`):

```sh
nvix() { nvi "$@" | xargs -0 -r env; }
```
### PowerShell

The Windows build defaults to `--format powershell`, emitting `$env:` assignments followed by a call-operator invocation.
PowerShell evaluates the emitted script: `Out-String` joins nvi's output back into a single string (PowerShell splits native stdout into lines, which would break multiline values), and `Invoke-Expression` executes it.

```powershell
nvi [flags|command] -- [command] | Out-String | Invoke-Expression
```

For day-to-day use, you may want to add a function to your PowerShell `$PROFILE`:

```powershell
function nvix { nvi @args | Out-String | Invoke-Expression }
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

## Flags

| Flag | Alias | Parameters | Description |
| --- | --- | --- | --- |
| `--dry-run` | `-d` | | Prints parsed flags, scan results, file tokens, and the parsed ENVs to stderr. |
| `--files` | `-f` | one or more paths | Parses `.env` files in sequential order (requires at least 1 `.env` file). Later files override earlier ones. Paths must be relative to the current directory, must not escape it, and must contain the `.env` extension. |
| `--format` | `-F` | `nul` or `powershell` | Formats ENVs for the downstream consumer. Defaults to `nul` on Unix and `powershell` on Windows (chosen at compile time per target). |
| `--help` | `-h` | | Prints usage help to stdout and exits with 0. |
| `--ignored` | `-i` | one or more keys | Ignores keys that `scan` may add to the required ENV list (e.g. `NODE_ENV`, which is typically injected at runtime). |
| `--required` | `-r` | one or more keys | Requires keys that must exist with non-empty values after parsing all `.env` files; exits with an 1 (operational error) with a list of keys that are undefined. |
| `--scan` | `-s`| one or more file extensions | Recursively scans \*.`<ext>` files for environment-variable accessors and sets the ENV required list.† |
| `--version` | `-v` | | Prints version info to stdout and exits with 0. |
| `--` | | command tokens | An end-of-options delimiter followed by a `<command>` (eg. `npm run dev`). Remains untouched and is emitted with ENVs for a downstream consumer to run. |

Unrecognized flags or arguments are usage errors.

> [!NOTE]
> † Without a `--` command, scan will only report what it finds and exit. With a `--` command, scan sets the found ENV keys to the required ENVs list.

## Usage examples

```sh
# multiple files; later files override earlier ones
nvi --files .env .env.local -- npm start | <consumer>

# require keys to be present
nvi --files .env --required API_KEY DATABASE_URL -- cargo run | <consumer>

# shows a dry run of what was scanned, tokenized, and parsed (prints to stderr)
nvi --files .env --scan ts --dry-run

# require every env key referenced in py source files to be present
nvi --files .env --scan py -- python main.py  | <consumer>

# shell expansion inside the command (single-quote so your shell doesn't expand first)
nvi --files .env -- sh -c 'echo "$MESSAGE"' | <consumer>
```
Values are passed through byte-exact without quoting or escaping: spaces, quotes, `$`, and even embedded newlines (multiline values) survive intact.

### Exit codes

- `0` - Success: emits ENVs for a downstream consumer or prints information and exits (help, scan, version)
- `1` - Operational failures: out of memory, file unreadable, parser errors, or required keys are undefined
- `2` - Usage errors: flags missing required params, invalid flags/params, or a missing `--` command

The exit code of *your command* will be reported by the downstream consumer, not by `nvi`.

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
> An environment-variable will be detected by *how it's accessed* and not by how it's spelled (indepedent of its casing, prefix, or suffix). That said, ideally, ENVs should be UPPER_CASE_SNAKE_CASE.

> [!CAUTION]
> Dynamic keys (`const key = "DATABASE_URL"; process.env[key]`), destructured variables (`const { DATABASE_URL } = process.env`), and aliased accessors (`const e = process.env; e.DATABASE_URL`) cannot be detected by the scanner without a per-language AST (which this tool avoids) and therefore won't be found.

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
- Zig -> `zig`

#### Scan Usage Examples

```sh
# scans and reports matching ENVs in .mjs and .ts files, then exits
nvi --scan mjs ts

# collects scanned keys to be required and defined before 'npm run dev' command is emitted
nvi --scan mjs --files .env -- npm run dev | xargs -0 -r env

# excludes runtime-injected ENVs found within the 'npm run dev' command environment
nvi --scan mjs --ignored NODE_ENV --files .env -- npm run dev | xargs -0 -r env
```

Notes:

- Extensions must be written as `ext` and not `.ext` or `*.ext`.
- Extensions with no known accessor patterns are usage errors.
- Dot-directories (eg. `.git`, `.next`, `.venv`, and so on) and common dependency/cache/build-output directories (eg. `node_modules`, `__pycache__`, `zig-out`, and so on) are ignored. Symlinked directories are not followed.
- When a command is present, scanned keys that are not defined exit with code `1` (usage error), the same as `--required` failures.

## `.env` file syntax

Here are some examples of how ENVs can be defined in an `.env` file:

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

# a hash sign after a key is a literal '#' (not comment)
CHANNEL=#why-is-this-bug-occuring

# backslash-newline continues the value (interpolation keys still work on any line)
SSH_PRIVATE_KEY=-----BEGIN RSA PRIVATE KEY-----\
MIIEpAIBAAKCAQEA2x5s8K9vN3pQ7mK8vL2d5pJ9mX6kL8qR3wT9uV5sZ2aB4cD\
oqRosTouVoaV1EthzxeIRx7pPoqR9sTiuVcwXjyZiBvcDj0FlgHgiJjlLjmNjoP\
owKBAQDZ2sX7pPoqRisTiuVcwXjyZiBvcDj0FlgHgiJjlLjmNjoPoqRosTouVoaV\
3EthzxeIRx7pPoqR9sTiuVcwXjyZiBvcDj0FlgHgiJjlLjmNjoPoqRosTouVoaV\
-----END RSA PRIVATE KEY-----
# when there's no backslash and just a new-line or EOF, then that indicates the end of a multiline value
```
> [!NOTE]
> Interpolated keys resolve first from the shell environment and then from keys parsed earlier (including earlier `.env` files specified by `--files`).
> Undefined key interpolations or keys with empty values will exit with an error.

## Testing

To run tests, use the following `make` or `zig build` commands:
```sh
make test
# zig build test --summary all

make unit
# zig build unit --summary all

make integration
# zig build integration --summary all
```

## Security model

`nvi` doesn't perform execution operations (like `execl`, `execlp`, `execle`, and so on), no process spawning nor shell invocation. It will only parse the `.env` files you provide and write ENVs to stdout.
Process execution happens entirely in the downstream consumer you choose (`xargs`/`env` or PowerShell), with the command tokens you've typed. For PowerShell, values are emitted inside single-quoted strings (the only escape being `''`), so values cannot break out of string context into executable position.

### [Contributing](https://github.com/mattcarlotta/nvi-bin/blob/main/CONTRIBUTING.MD)
### [License](https://github.com/mattcarlotta/nvi-bin/blob/main/LICENSE.md)
