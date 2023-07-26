# nvi bin

A stand-alone .env file parser for interpolating and assigning multiple .env files into a process.

## Quick Links

[Requirements](#requirements)

[Installation](#installation)
  - [Custom CMake Compile Flags](#custom-cmake-compile-flags)

[Usage](#usage)

[Flags](#flags)

[Configuration File](#configuration-file)

[Examples](#examples)

[FAQs](#faqs)
  - [How do I uninstall the executable?](#how-do-i-uninstall-the-executable)
  - [What are the rules for defining or interpolating keys?](#what-are-the-rules-for-defining-or-interpolating-keys)
  - [What Operating Systems are supported?](#what-operating-systems-are-supported)
  - [What are the nvi configuration file specs?](#what-are-the-nvi-configuration-file-specs)
  - [Can I manually assign parsed ENVs to a process?](#can-i-manually-assign-parsed-envs-to-a-process)
  - [How do I read the debug details?](#how-do-i-read-the-debug-details)

[Troubleshooting](#troubleshooting)

[Contributing](CONTRIBUTING.md)

## Requirements

The following requirements must be present in order to build from source:
- gcc v11.3.0+
- clang v14.0.x
- cmake v3.26.3+
- make v3.8.x+
- clangd v14.0.x (optional for linting)
- clang-format v14.0.x (optional for formatting)

You can determine if you're using the correct versions by:
- Running `which <requirement>` that should output a executable path, if not, then it's not installed and will require platform specific installations.
- Running `<requirement> --version` that should output a executable version equal to or above the required version.

## Installation
```DOSINI
# if you plan to run the unit tests, add the "--recursive" flag to install test dependencies
git clone git@github.com:mattcarlotta/nvi.git nvi

cd nvi

# if you want to use custom compile flags see below
cmake .

# optional command to build the source quickly
# swap NUMBER_OF_CPU_CORES for the number that is printed using one of the commands below:
# GNU/LINUX: grep -m 1 'cpu cores' /proc/cpuinfo
# MAC OS: sysctl -n hw.ncpu
# cmake --build . -j NUMBER_OF_CPU_CORES

sudo make install
```

### Custom CMake Compile Flags

The following custom compile flags can be set for `cmake`:
- `-DCOMPILE_SRC=ON|OFF` this compiles the source files within `src` to a `nvi` executable (default: ON)
- `-DCOMPILE_TESTS=ON|OFF` this compiles the source files within `tests` to a `tests` executable (default: OFF)
- `-DINSTALL_BIN_DIR=/custom/directory/path` this will override the executable installation directory when running `sudo make install` (default: /usr/local/bin)

The following represents the default `cmake` settings:
```DOSINI
cmake -DCOMPILE_SRC=ON -DCOMPILE_TESTS=OFF -DINSTALL_BIN_DIR=/usr/local/bin .
```

## Usage

Navigate to a project that contains one or many `.env` files, then type:
```DOSINI
nvi <flag> <arg>
```

## Flags
All flags below are optional. Short form (`-`) and long form (`--`) flags are supported and can be mixed if desired.

If no flags are assigned, then an `.env` (that is named ".env") located at the root directory will be parsed.

- `-c` | `--config`: Specifies which environment config to load from the .nvi file. (ex: --config dev)‡
- `-de` | `--debug`: Specifies whether or not to log debug details. (ex: --debug)
- `-d` | `--dir`: Specifies which directory the .env files are located within. (ex: --dir path/to/envs)
- `-e` | `--exec`: Specifies which command to run in a separate process with parsed ENVs. (ex: --exec node index.js)
- `-f` | `--files`: Specifies which .env files to parse separated by a space. (ex: --files test.env test2.env)
- `-r` | `--required`: Specifies which ENV keys are required separated by a space. (ex: --required KEY1 KEY2)
- `-h` | `--help`: Displays this help information.

‡ When a "-c" or "--config" flag is present, then "debug", "dir", "exec", "files", and "required" flags are ignored as they should be defined within a [configuration file](#configuration-file).


## Configuration File
Instead of manually typing out flags and arguments in the CLI, there is support for placing them in an `.nvi` configuration file.

The configuration file is a [TOML](https://toml.io/en/)-like formatted file that contains...
- An `[environment]` name that defines the following optional properties: 
  - **debug**: boolean (default: `false`) 
  - **dir**: string (default: `""`)
  - **exec**: string (default: `""`)
  - **files**: string[] (default `[".env"]`) 
  - **required**: string[] (default `[]`)

The following represents an example `.nvi` configuration:
```toml
[dev]
debug = true
dir = "path/to/custom/dir"
files = [ ".env", "base.env", "reference.env" ]
exec = "bin test"
required = [ "TEST1", "TEST2", "TEST3" ]

[staging]
files = [ ".env" ]
required = [ "TEST1" ]
```

To target an environment within the configuration file, simply use the `-c` or `--config` flag followed by the environment name:
```DOSINI
nvi -c dev
# or
nvi --config staging
```

Please read [this](#what-are-the-nvi-configuration-file-specs) for config file specs.

### [Examples](/examples)

Click on the link above for language specific examples, otherwise, here are some basic use cases:

Parsing an `example.env` file from a custom directory with debug logging:
```DOSINI
nvi --files example.env --dir dist/client --debug
```

Parsing one or many `.env` files from a [.nvi](envs/.nvi#L5-L10) configuration file typically located at a project's root directory:
```DOSINI
nvi --config standard
```

Parsing an `.env` file, checking the parsed ENVs for required keys, and then, if good, applying those ENVs to a spawned "npm" child process:
```DOSINI
nvi --files .env --exec npm run dev --required KEY1 KEY2
```

## FAQs

### How do I uninstall the executable?
If you'd like to remove (uninstall) the executable, simply type:
```DOSINI
sudo rm $(which nvi)
```

### What are the rules for defining or interpolating keys?
There are really only 3 simple rules that must be followed:
- To interpolate a key's value into another key, use `${NAME_OF_KEY}`, where `NAME_OF_KEY` represents the name of an ENV:
```DOSINI
NAME_OF_KEY=1
# interpolates a value from a key above
REFERENCE=${NAME_OF_KEY}
# interpolates a value from a shell environment variable
ENV_HOME_PATH=${HOME}
```
- Keys can only reference other keys if they're already defined in the shell environment (use `printenv` in a terminal for a list of shell environment variables) or they've already been parsed by the nvi parser (this means you can cross-reference keys in other .env files, but they must have already been parsed in order to be interpolated).
- Multi-line keys must end with `\↵` (a back-slash followed by a new line or carriage return). To end a multi-line key, just use a new line without a back-slash.

Other things to note:
- Keys should not include interpolated values, they'll be ignored and kept as is. For example: `TEST${KEY}=hello` retains the same key: `"TEST${KEY}": "hello"`.
- Only double quotes are escaped for printing values to `stdout`, for example `"hello"` will be printed as `"\"hello\""`.
- Empty spaces are retained `      hello        world      ` and don't require any quotes.

### What Operating Systems are supported?
Currently, GNU linux and Mac OS (v13+ although older versions that support C++17 may work as well). For Windows support, please visit this [documentation](https://i.imgur.com/MPGenY1.gif).

### What are the nvi configuration file specs?
This `.nvi` configuration file attempts to give you the flexibility and ease-of-use of defining flags and arguments under an environment configuration. 

Therefore, while the config file parser is flexible, it is **NOT** a TOML-complaint parser and has some rules.

The configuration file must:
- be named `.nvi`
- not contain spaces within the `[environment]`'s name
- not contain spaces within a `files` .env name nor within the `required` keys; instead, files/keys should use underscores: `example_1`
- not contain comments after a configuration `key = value # comment` option
- not contain empty lines between a config `key = value` option; empty lines after the last config option are okay
- not contain multi-line arguments

Click [here](envs/.nvi) to view valid vs invalid formatting configurations.

### Can I manually assign parsed ENVs to a process?
Yes! If you don't use an `-e` or `--exec` or an `execute` command in a configuration file, then nvi will print out a stringified JSON result of ENVs to [stdout](https://www.computerhope.com/jargon/s/stdout.htm). 

Unfortunately, this means you'll have to manually pipe and parse stringified JSON from `stdin`, and then assign them to the process for whatever language or framework that you're using. As such, this feature is available to you, but there are expected drawbacks:
- requires language or framework specific code (what this project aims to mitigate)
- reading from `stdin` may not be possible
- reading from `stdin` may be asynchronous and there's no guarantee that when a program/process runs that the ENVs will be defined before they are used
- parsing stringified JSON will more than likely require a 3rd party package

As such, while this feature is available, it's not recommended nor going to be supported by this project. Nevertheless, here's an example of how to pipe ENVs into a [node process](/examples/node) using [stdin](/examples/node/stdin.mjs).

### How do I read the debug details?
To read the debug details, let's examine the following debug message:
```DOSINI
[nvi] (Parser::log::INTERPOLATION_WARNING) [.env:88:21] The key "INTERP_ENV_FROM_PROCESS" contains an invalid interpolated variable: "TEST". Unable to locate a value that corresponds to this key.
```
- The class that spawned the message: `Parser`
- The class method that logged the message: `log`
- The type of message that is being logged: `INTERPOLATION_WARNING`
- The file that is referenced: `.env`
- The line within the file that is referenced: `88`
- The byte within the line that is referenced: `21`
- The message: `The key "..." contains an invalid interpolated variable...`

In layman's terms, this debug message is stating that a key's value within an `.env` file contains an interpolated key `${KEY}` (`TEST`) on line 88 at byte 21 that doesn't match any ENV keys in the shell environment nor any previously parsed ENV keys.

The solution to the above is to either ensure the ENV key exists within the shell environment before running the executable or only reference keys in the .env file after they've been parsed (.env files are parsed top-down, therefore keys can only reference other keys above itself, and .env files are parsed left to right, therefore keys can only reference other keys in files that have been parsed before itself).

Not all debug logs will have all the details above, but they will generally follow this pattern.


## Troubleshooting

⚠️ Please note that some operating systems (like Mac OS) may not have a "/usr/local/bin" directory nor use it as a search `PATH` for binaries.

To fix this, create the directory (if you're wary of touching `/usr`, then you may want to use `/opt` or `/opt/bin` instead):
```DOSINI
sudo mkdir -p /usr/local/bin
```

Then, add this directory path to the shell's `PATH` (swap in whichever profile your shell uses, eg. `.bash_profile`, `.bashrc` or `.zsh`):
```DOSINI
echo 'export PATH="$PATH:/usr/local/bin"' >> ~/.bash_profile
```

Then, source the change for your shell profile:
```DOSINI
source ~/.bash_profile
```

To ensure the executable is found, type the command below and you should see the nvi executable path: 
```DOSINI
which nvi
# /usr/local/bin/nvi
```
