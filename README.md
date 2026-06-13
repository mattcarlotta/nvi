# nvi

A fast, cross-platform, exec-free, RegEx-free `.env` parser and emitter.

- 0 dependencies and language agnostic
- parses one or more `.env` files
- handles `${KEY}` interpolations
- supports multiline values via `\` delimiter
- can validate required keys before execution
- can scan source files for `*_ENV` references and mark them as required
- reads .env files and emits ENVs to stdout

```
nvi --files .env -- <command> | xargs -0 env
```

## How it works

1. `nvi` parses and interpolates the given `.env` file(s) into `KEY=value` pairs
2. It writes each pair to stdout, followed by the command tokens that appear after `--`
3. A downstream consumer (like `xargs -0 env` on Unix or `Invoke-Expression` on Windows) then sets the variables and runs the command

All diagnostics (errors, warnings, `--debug` output) go to **stderr**.

## Flags

| Flag | Alias | Parameters | Description |
| --- | --- | --- | --- |
| `--files` | `-f` | one or more paths | `.env` files to parse, in order. Later files override earlier ones. Paths must be relative to the current directory, must not escape it, and must contain `.env`. Defaults to `.env`. |
| `--ignored` | `-i` | one or more keys | Keys that `scan` should not add to the required ENV set (e.g. `NODE_ENV`, which is typically injected at runtime by tooling rather than defined in a `.env` file). Only meaningful together with `scan`. |
| `--required` | `-r` | one or more keys | Keys that must exist with non-empty values after parsing. `nvi` exits with an error listing any that are missing. |
| `--format` | `-F` | `nul` or `powershell` | Output format. Defaults to `nul` on Unix and `powershell` on Windows (chosen at compile time per target). |
| `--debug` | `-d` | none | Print parsed flags, tokens, scan results, and the resolved env listing to stderr. The env listing follows the active `--format`, so it previews the exact pair syntax that will hit stdout. |
| `--` | | command tokens | End-of-options delimiter. Everything after it is passed through to stdout untouched, as the command for the downstream consumer to run. Required (except for standalone `scan`, `help`, and `version`). |

Unrecognized flags (and their parameters) are warned about on stderr and ignored.

## Commands

| Command | Parameters | Description |
| --- | --- | --- |
| `scan` | one or more file extensions | Recursively scans `*.<ext>` files from the current directory for `*_ENV` key references and marks them as required.† |
| `help` | | Prints usage help. |
| `version` | | Prints version. |

† Without a `--` command, it'll report what it finds and exits. With a `--` command, the found ENV keys are added to the required ENV set and must be defined before the ENVs and command are emitted.

Unrecognized commands are warned about on stderr and ignored.

### Exit codes

- `0` - Parsed and emitted successfully
- `1` - Operational failure: file unreadable, parse error, missing required keys, or output write failure (details on stderr)
- `2` - Usage error: invalid flags or missing `--` command (details on stderr)

The exit code of *your command* is reported by the downstream consumer (`xargs`), not by `nvi`.

## Scanning for ENV references

`scan` walks the file tree from the current directory and collects identifiers that end in `_ENV` (e.g. `API_KEY_ENV`, `DATABASE_URL_ENV`) from files matching the given extensions.

```sh
# report every *_ENV reference in .mjs and .ts files, then exit
nvi scan mjs ts

# check mode: scanned keys must be defined before the command is emitted
nvi scan mjs --files .env -- npm run dev | xargs -0 -r env

# exclude runtime-injected keys from the requirement
nvi scan mjs --ignored NODE_ENV --files .env -- npm run dev | xargs -0 -r env
```

Notes:

- Extensions may be written as `ext`, `.ext`, or `*.ext`. **Quote glob spellings** (`'*.ext'`) or pass bare extensions; unquoted globs are expanded into filenames by your shell before `nvi` runs.
- Matches are UPPERCASE_SNAKE identifiers ending in `_ENV`. Partial matches inside larger identifiers (`THING_ENVIRONMENT`, `my_API_ENV`) are skipped.
- These directories are never descended into: `.git`, `node_modules`, `zig-out`, `.zig-cache`, `zig-cache`, `dist`, `build`, `vendor`, `target`, `coverage`. Symlinked directories are not followed.
- The report goes to stderr; `--debug` adds a per-file listing of every reference with its line and byte position.
- In scan mode, missing scanned keys exit with code `1`, the same as `--required` failures, and the command is never emitted.

## `.env` syntax

```dotenv
# comments start with a hash
MESSAGE=hello                       # a literal value
GREETING=${MESSAGE} world           # interpolation from a KEY in an .env file or from the process environment
BASE64_OK=abc==                     # an equal sign '=' after a key is literal
PRICE=$5                            # '$' without braces '{' is a literal
SSH_KEY=ssh-rsa ABC\                # backslash-newline continues the value; interpolation also works on any line
De1F2\
g3HI${MESSAGE}== user@example.com
```

Interpolated keys resolve first from the process environment and then from keys parsed earlier (including earlier `--files`).
Undefined interpolations and keys with empty values are skipped with a warning.

## Building

Requires Zig `0.16.0` or later. And optionally ZLS for linting.

```sh
# debug build (default): zig-out/bin/nvi
zig build

# run all of test suites
zig build test --summary all

# build & install somewhere on your $PATH
zig build --release=small --prefix ~/.local
```
### Verify installation

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

### Cross-compiling

Zig cross-compiles every target from any host without extra toolchains.
Please note that the Windows binary defaults to `--format powershell` automatically; this is baked in at compile time.

```sh
zig build --release=small -Dtarget=aarch64-macos
zig build --release=small -Dtarget=x86_64-macos
zig build --release=small -Dtarget=x86_64-linux-musl
zig build --release=small -Dtarget=aarch64-linux-musl
zig build --release=small -Dtarget=x86_64-windows
```

The `-musl` Linux targets produce fully static binaries that run on any distribution.

## Running

### Unix (Linux, macOS)

`nvi` emits NUL-delimited ENVs: each `KEY=value` pair, then each command token. `xargs -0` splits the ENVs and hands them to `env`, which sets the variables and runs the command.

```sh
nvi --files .env -- <command> | xargs -0 env
```

More examples:

```sh
# multiple files; later files override earlier ones
nvi --files .env .env.local -- npm start | xargs -0 env

# require keys to be present
nvi --files .env --required API_KEY DATABASE_URL -- cargo run | xargs -0 env

# require every *_ENV key referenced in source files to be present
nvi scan mjs --ignored NODE_ENV --files .env -- npm start | xargs -0 -r env

# print a single resolved variable
nvi --files .env -- printenv MESSAGE | xargs -0 env

# inspect the full child environment
nvi --files .env -- env | xargs -0 env

# shell expansion inside the command (single-quote so your shell doesn't expand first)
nvi --files .env -- sh -c 'echo "$MESSAGE"' | xargs -0 env

# debug what was parsed (stderr only)
nvi --files .env --debug -- echo $?
```
For day-to-day use, you may want to add a function to your shell profile (eg. `~/.zshrc`, `~/.bashrc`):

```sh
nvix() { nvi "$@" | xargs -0 -r env; }
```

NUL delimiting means values pass through byte-exact without quoting or escaping: spaces, quotes, `$`, and even embedded newlines (multiline values) survive intact.

### Windows (PowerShell)

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
# preview the PowerShell ENV emission
nvi --files .env --format powershell --debug -- <command>

# preview the Unix ENV emission
nvi --files .env --format nul --debug -- <command>
```

## Security model

`nvi` performs no exec-like operations: no `exec`, no process spawning, no shell invocation. It reads the `.env` files you name and writes bytes to stdout.
Process execution happens entirely in the downstream consumer you choose (`xargs`/`env` or PowerShell), with the command tokens you typed passed through verbatim.
On the PowerShell path, values are emitted inside single-quoted strings (the only escape being `''`), so values cannot break out of string context into executable position.

### [Contributing](https://github.com/mattcarlotta/nvi-bin/blob/main/CONTRIBUTING.MD)
### [License](https://github.com/mattcarlotta/nvi-bin/blob/main/LICENSE.md)
