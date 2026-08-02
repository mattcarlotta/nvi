# nvi

A fast and minimal cross-platform CLI `.env` parser, environment-variable scanner and emitter.

- 0 dependency
- Language and framework agnostic (replaces language specific env packages)
- Sequentially verifies and parses one or more `.env` files
- Supports `${KEY}` interpolations, `#` comments, `'` and `"` quotes, and `\` delimited multiline values
- Scans project files for environment-variable references across many [languages](#supported-file-extensions) and marks them as required
- Checks required environment-variables are defined before command execution
- Supports ignoring environment-variables that may be set at run-time
- Loads flags from a [`.nvi` config file](#nvi-config-file)

![animated gif of using nvi commands](nvi.gif)

## Table of contents

- [Installation](#installation)
  - [Building and installing from source](docs/BUILD.md)
  - [POSIX (Linux, macOS, WSL) installation script](#posix-installation-script)
  - [PowerShell (Windows) installation script](#powershell-installation-script)
  - [Verifying your PATH](#verifying-your-path)
    - [POSIX](#posix-path)
    - [PowerShell](#powershell-path)
- [Running](#running)
  - [POSIX](#run-on-posix-linux-macos-wsl)
  - [PowerShell](#run-on-powershell-windows)
- [Flags](#flags)
- [Usage examples](#usage-examples)
  - [Exit codes](#exit-codes)
- [`.nvi` config file](#nvi-config-file)
- [Scanning for ENV keys](#scanning-for-env-keys)
  - [Supported file extensions](#supported-file-extensions)
  - [Scan usage examples](#scan-usage-examples)
- [`.env` file syntax](#env-file-syntax)
- [Development](#development)
- [Security](#security)
- [Contributing](CONTRIBUTING.md)
- [License](LICENSE.md)

## Installation

Pick one of the following installation options:

- **[Build and install from source](docs/BUILD.md)** (best compatibility)
- **Installation scripts** (quickest; fetches a precompiled binary and wires it up to your shell): [POSIX](#posix-installation-script) or [PowerShell](#powershell-installation-script)
- **[Precompiled binary](https://github.com/mattcarlotta/nvi/releases/)** (manual; extract it and place it within a directory [recognized by your shell](#verifying-your-path))

> [!NOTE]
> There are no prebuilt macOS x86_64 or Linux musl aarch64 binaries. On those platforms, the install script exits with an error and you'll need to [build and install from source](docs/BUILD.md).

### POSIX installation script

By default, the binary will be installed to `$HOME/.local/bin`, the directory will be appended to your user `$PATH`, and a `nvix` function will be appended to your shell `$PROFILE`:
```sh
curl -fsSL https://raw.githubusercontent.com/mattcarlotta/nvi/main/install.sh | sh
```

Once the script has completed, you must source (reload) your current shell for the changes to go into effect:
```sh
source <PROFILE>
```

<details>
<summary>Custom script installations</summary>

Download the script:
```sh
curl -fsSL https://raw.githubusercontent.com/mattcarlotta/nvi/main/install.sh -o install.sh
```

| Flag | Env | Default | Description |
| --- | --- | --- | --- |
| `-v, --version <tag>` | `NVI_VERSION` | `latest` | Release tag to install (eg. `v0.1.2`). |
| `-d, --dir <path>` | `NVI_INSTALL_DIR` | `$HOME/.local/bin` | Install destination. |
| `--libc <gnu\|musl>` | `NVI_LIBC` | auto-detected | Linux libc flavor. |
| `--no-profile` | | | Print the profile block instead of appending it. |
| `--uninstall` | | | Remove the binary and the profile block. |
| `-h, --help` | | | Print usage and exit. |

Run with custom flag options:
```sh
sh install.sh [flags]
```

Uninstall:
```sh
sh install.sh --uninstall
```

</details>

### PowerShell installation script

Requires Windows 10 or newer, on Windows PowerShell 5.1 or PowerShell 7+.

By default, the binary will be installed to `$env:LOCALAPPDATA\Programs\nvi\bin`, the directory will be appended to your user `Path`, and a `nvix` function will be appended to your `$PROFILE`:
```powershell
irm https://raw.githubusercontent.com/mattcarlotta/nvi/main/install.ps1 | iex
```

Once the script has completed, you must source (reload) your current shell for the changes to go into effect:
```powershell
. $PROFILE
```

> [!NOTE]
> Windows PowerShell 5.1 and PowerShell 7+ use different `$PROFILE` paths (`Documents\WindowsPowerShell\` and `Documents\PowerShell\`). The block is written to the profile of whichever host you run the script from, so run it from the shell you actually use.

<details>
<summary>Custom script installations</summary>

Download the script:
```powershell
irm https://raw.githubusercontent.com/mattcarlotta/nvi/main/install.ps1 -OutFile install.ps1
```

| Parameter | Env | Default | Description |
| --- | --- | --- | --- |
| `-Version <tag>` | `NVI_VERSION` | `latest` | Release tag to install (eg. `v0.1.2`). |
| `-InstallDir <path>` | `NVI_INSTALL_DIR` | `$env:LOCALAPPDATA\Programs\nvi\bin` | Install destination. |
| `-NoPathUpdate` | | | Skip the user `Path` update. |
| `-NoProfileUpdate` | | | Print the profile block instead of appending it. |
| `-Uninstall` | | | Remove the binary, its `Path` entry, and the profile block. |

Run with custom parameters:
```powershell
.\install.ps1 [parameters]
```

Uninstall:
```powershell
.\install.ps1 -Uninstall
```

> [!NOTE]
> Running a downloaded `.ps1` may be blocked by the execution policy. Either unblock the single file with `Unblock-File .\install.ps1`, or allow scripts for the current session only with `Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass`. The `irm | iex` form isn't affected, since nothing is executed from disk.

</details>

### Verifying your PATH

Before placing or installing a binary into a directory, you must ensure the destination is recognized by your shell.

#### POSIX PATH

First, list the `$PATH` directories along with their owner, and pick one owned by `$USER` and not by `root`:

macOS:
```sh
echo $PATH | tr ':' '\n' | xargs -I{} sh -c 'printf "%-50s %s\n" "{}" "$(stat -f "%Su:%Sg" "{}" 2>/dev/null)"' | nl
```

GNU Linux:
```sh
echo $PATH | tr ':' '\n' | xargs -I{} sh -c 'printf "%-50s %s\n" "{}" "$(stat -c "%U:%G" "{}" 2>/dev/null)"' | nl
```

If there aren't any `$USER` owned bin directories, create a local one:
```sh
mkdir -p $HOME/.local/bin
```

Then update your shell profile's `$PATH`.

zsh (`~/.zshrc`):
```sh
typeset -U path PATH
path=("$HOME/.local/bin" $path)
```

bash (`~/.bashrc` or `~/.bash_profile` on macOS):
```sh
export PATH="$HOME/.local/bin:$PATH"
```

Then source (reload) the profile:
```sh
source <PROFILE>
```

#### PowerShell Path

First, list the current `Path` entries:
```powershell
$env:Path -split ';'
```

If the destination `<DIR>` isn't listed, add it (eg. `C:\tools\bin`), then close and reopen PowerShell:
```powershell
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";<DIR>", "User")
```

## Running

### Run on POSIX (Linux, macOS, WSL)

The POSIX build defaults to `--format nul`, emitting NUL-delimited `KEY=value\0` assignments followed by the command tokens.

If it doesn't already exist, then you'll need to add a `nvix` function to your shell profile.

For zsh (`~/.zshrc`):
```zsh
nvix() {
  local out
  out="$(nvi "$@")" || return $?
  [[ -n "$out" ]] || return 0
  env ${(0)out}
}
```

For bash 4.4+ (`~/.bashrc`):
```bash
nvix() {
  local args=()
  mapfile -d '' -t args < <(nvi "$@")
  wait "$!" || return $?
  ((${#args[@]})) || return 0
  env "${args[@]}"
}
```

> [!IMPORTANT]
> In the bash function, `wait "$!"` recovers nvi's exit code from the process substitution and must not be dropped: without it a failed parse looks successful.

<details>
<summary>For 3.2 bash (macOS) and other POSIX shells</summary>

For bash versions 3.2 and below, you'll need to pipe to `xargs`:
```sh
# GNU xargs (Linux, WSL)
nvix() { nvi "$@" | xargs -0 -r env; }

# BSD xargs (macOS)
nvix() { nvi "$@" | xargs -0 env; }
```

The zsh and bash 4.4+ functions build the same `KEY=value ... command` vector `xargs` would (zsh variables can hold NUL bytes; bash array *elements* sit between the NULs), but run `env` as a direct child of your shell. That makes `ctrl+c` on a long-running command behave exactly like running it directly, avoiding the dangling partial line (`^C%` in zsh) the `xargs` pipeline leaves when SIGINT returns the prompt before the command finishes shutting down.

</details>

Then source (reload) the profile (eg. `~/.zshrc`, `~/.bashrc`, or `~/.bash_profile`):
```sh
source <PROFILE>
```

To verify it's available, run:
```sh
type nvix
```

### Run on PowerShell (Windows)

The Windows build defaults to `--format powershell`, emitting `$env:` assignments followed by a call-operator invocation.

```powershell
nvi [flags] -- [command] | Out-String | Invoke-Expression
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

| Flag | Description |
| --- | --- |
| `-d, --dry-run` | Prints results to stderr and exits with 0. |
| `-f, --files <file> ...`| Parses one or more `.env` files in sequential order. |
| `-F, --format <format>` | Formats ENVs for the consumer (formats: `nul` or `powershell`). |
| `-h, --help` | Prints usage help to stdout and exits with 0. |
| `-i, --ignored <KEY> ...` | Ignores a list of keys that a `scan` may add to the required ENV list. |
| `-r, --required <KEY> ...` | Requires a list of keys that must be defined after parsing. |
| `-R, --reveal` | Reveals ENV values in a dry-run; otherwise, they'll be hidden (`*****`). |
| `-s, --scan <ext> ...` | Recursively scans [`<ext>`](#supported-file-extensions) files for environment-variable accessors. † |
| `-t, --threads <1-255>` | Number of threads to use when scanning files (max: CPU thread count). †† |
| `-v, --version` |  Prints version info to stdout and exits with 0. |
| `@<config>` | Loads flags from a [`.nvi` config file](#nvi-config-file) (eg. `@development.nvi`). |
| `--` <command> | An end-of-options delimiter followed by a `<command>` (eg. `npm run dev`). |

> † without a `--` command, scan will only report what it finds and exit (must include **--dry-run**); with a `--` command, scan sets the found ENV keys to the required ENVs list.

> †† using more threads than your hardware or software can handle will degrade scanning performance

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
nvi --files .env --scan ts --dry-run 2> nvi.log; less nvi.log

# require every env key referenced in py source files to be present
nvi --files .env --scan py -- python main.py  | <consumer>

# POSIX shell expansion inside the command (single-quote so your shell doesn't expand first)
nvi --files .env -- sh -c 'echo "$MESSAGE"' | <consumer>
```

### Exit codes

- `0` - Success: emits ENVs for a consumer or prints information and exits (help, scan, version)
- `1` - Operational failures: out of memory, file unreadable, parser errors, or required keys are undefined
- `2` - Usage errors: flags missing required params, invalid flags/params, or a missing `--` command

The exit code of *your command* will be reported by the consumer, not by `nvi`.

## `.nvi` config file

Just like `.env` files, you may use one or many `.nvi` config files to load project and/or environment specific flags.

Usage:
```sh
nvi @<path> -- <command> | <consumer>
```

Example config:
```sh
# local.nvi
--files .env .env.local
--format nul
--scan ts tsx mjs
--ignored NODE_ENV CI
--threads 4
```

You'll still have the option to append or override flags after a config file (except for flags that don't have parameters, like: `--dry-run`):
```sh
# the local.nvi config (above) supplies the defaults, but the flags
# specified afterward append .env.production to files and override the format
nvi @local.nvi --files .env.production -F powershell -- <command> | <consumer>
```

Rules:
- Supports loading a single `.nvi` file (referencing other `.nvi` configs is unsupported).
- Flags and parameters must be defined on the same line.
- A `--` command is not allowed inside a config file; commands stay within the command line, where it'll be handled by the consumer.
- An empty or comment-only config file is an error.

## Scanning for ENV keys

Providing a `-s` or `--scan` flag followed by one or many file `ext`s, walks a project's file tree from the current directory and, for each file matching the given extensions, looks for the environment-variable accessors of that file's language.

For example, every line below would be recognized and yield the key `DATABASE_URL`:

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

The following will NOT be detected by the scanner...

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

### Supported file extensions
- C: `c`, `h`
- Clojure: `clj`, `cljs`, `cljc`
- Crystal: `cr`
- C++: `cc`, `cpp`, `cxx`, `hh`, `hpp`, `hxx`
- C#: `cs`
- D: `d`
- Dart: `dart`
- Elixir: `ex`, `exs`
- Erlang: `erl`, `hrl`
- Fortran: `f`, `f90`, `f95`, `f03`, `f08`, `for`
- F#: `fs`, `fsi`, `fsx`
- Go: `go`
- Gradle: `gradle`
- Groovy: `groovy`
- Haskell: `hs`, `lhs`
- Java: `java`
- JavaScript/TypeScript: `cjs`, `cts`, `js`, `jsx`, `mjs`, `mts`, `ts`, `tsx`
- Julia: `jl`
- Kotlin: `kt`, `kts`
- Lua: `lua`
- Nim: `nim`
- Nushell: `nu`
- Objective-C: `m`, `mm`
- OCaml: `ml`, `mli`
- Pascal/Delphi: `dpr`, `pas`, `pp`
- Perl: `pl`, `pm`, `t`
- PHP: `php`
- PowerShell: `ps1`, `psm1`, `psd1`
- Python: `py`, `pyi`, `pyw`
- R: `r`
- Ruby: `gemspec`, `rb`, `rake`
- Rust: `rs`
- Scala: `sc`, `scala`
- Swift: `swift`
- Tcl: `tcl`
- V: `v`
- Visual Basic: `vb`
- YAML: `yaml`, `yml` †
- Zig: `zig`

> † YAML has no language-level accessor, so the scanner matches POSIX-style parameter expansion: `${KEY}` plus the operator forms `${KEY:-default}`, `${KEY:?err}`, etc.
> A bare `$KEY`, `$${KEY}`, and `${{ ... }}` expressions are ignored.

Notes:

- Extensions must be written as `ext` and not `.ext` or `*.ext`.
- Extensions with no known accessor patterns are usage errors.
- Dot-directories (eg. `.git`, `.next`, `.venv`, and so on) and common dependency/cache/build-output directories (eg. `node_modules`, `__pycache__`, `zig-out`, and so on) are ignored.
- Symlinked directories are not followed.

### Scan usage examples

```sh
# scans for matching ENVs within .mjs and .ts files using 4 threads, reports findings, then exits
nvi --scan mjs ts --threads 4 --dry-run

# collects scanned keys to be required and defined before the 'node index.js' command is emitted
nvi --scan mjs --files .env -- node index.mjs | <consumer>

# ignores runtime-injected ENVs often found within 'npm run dev' (node) environment
nvi --scan mjs --ignored NODE_ENV --files .env -- npm run dev | <consumer>
```

> [!IMPORTANT]
> There may be a hardware or software bottleneck with how many threads can be used at one time to scan files. A general rule of thumb is to start with 4 threads (if available) and then increase by 2.
> For example, if a CPU has 8 cores/16 threads, start with 4 threads, then 6, then 8... up to the max CPU thread count (16 threads).
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
| `nvi --scan ts tsx mjs cjs js jsx rs --threads 1 --dry-run` | 323.4 ± 7.0 | 310.3 | 341.9 | 8.78 ± 0.76 |
| `nvi --scan ts tsx mjs cjs js jsx rs --threads 4 --dry-run` | 101.3 ± 4.6 | 84.7 | 112.0 | 2.75 ± 0.26 |
| `nvi --scan ts tsx mjs cjs js jsx rs --threads 6 --dry-run` | 72.6 ± 3.3 | 66.0 | 78.7 | 1.97 ± 0.19 |
| `nvi --scan ts tsx mjs cjs js jsx rs --threads 8 --dry-run` | 60.4 ± 3.4 | 51.0 | 65.9 | 1.64 ± 0.17 |
| `nvi --scan ts tsx mjs cjs js jsx rs --threads 16 --dry-run` | 44.5 ± 5.0 | 33.1 | 52.8 | 1.21 ± 0.17 |
| `nvi --scan ts tsx mjs cjs js jsx rs --threads 32 --dry-run` | 36.9 ± 3.1 | 26.4 | 40.7 | 1.00 |

The test numbers above **ARE NOT** meant to be a measurement nor a comparison for how fast the scanner can run on a given system, but instead to showcase how a system can have file IO limitations past a certain number of threads.
For the MacBook Pro, more threads degraded scanning performance, whereas the desktop improved asymptotically (diminishing returns).
</details>

## `.env` file syntax

Here are some examples of how ENVs can be defined in an `.env` file:

```dotenv
# comments start with a hash (inline comments are not supported)

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

# single or double qoutes after a key and within a value are not stripped
# and are treated as literal characters
MESSAGE=she said "hello world" in death
RESPONSE=then he said 'goodbye my love' in life

# an explicitly empty quoted value after a key is allowed, but a bare 'KEY='
# without a value is an error
EMPTY_OK=""

# a POSIX shell-style export prefix is stripped
export EXPORTED=value

# a default POSIX shell-style ':-' value after a key is supported for an
# interpolated ${KEY} when the KEY is unset or empty
RETRIES=${MAX_RETRIES:-3}

# backslash-newline continues a multiline value
# an interpolation ${KEY} will still work on any same line
SSH_PRIVATE_KEY=-----BEGIN RSA PRIVATE KEY-----\
MIIEpAIBAAKCAQEA2x5s8K9vN3pQ7mK8vL2d5pJ9mX6kL8qR3wT9uV5sZ2aB4cD\
-----END RSA PRIVATE KEY-----
# when there's no backslash and just a new-line or EOF, then that
# indicates the end of a multiline value
```

- Keys must match `[A-Za-z_][A-Za-z0-9_]*`; anything else is a tokenizer error.
- Interpolated keys resolve first from the shell environment and then from any keys parsed from earlier `.env` files specified by `--files`.
- An undefined key interpolation without a `:-` fallback, a bare `KEY=` with no value, or a `--required` key that is undefined/empty after parsing are parser errors.

## Development

- [Building from source](docs/BUILD.md)
- [Testing and fuzzing](docs/DEVELOPMENT.md)

## Security

nvi doesn't use [exec](https://man7.org/linux/man-pages/man3/exec.3p.html) nor [regular expressions](https://man7.org/linux/man-pages/man3/regcomp.3.html). It also doesn't spawn processes nor invoke a shell.
It's purpose is to parse the `.env` files you provide, enforce size limits on what it parses, and write ENVs to stdout. Execution happens entirely in the consumer you choose.

See [SECURITY.md](SECURITY.md) for more information.

