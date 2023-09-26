# NVI

A stand-alone .env file parser for interpolating and assigning ENVs into a process locally or remotely.

## Quick Links

[Requirements](#requirements)

[Quick Installation](#quick-installation)
  - [Custom CMake Compile Flags](#custom-cmake-compile-flags)

[Custom Installations](#custom-installations)

[Usage](#usage)

[Flags](#flags)

[Configuration File](#configuration-file)

[Remote ENVs](#remote-envs)

[Man Documenation](docs/README.md)

[Examples](#examples)

[FAQs](#faqs)
  - [How do I uninstall the binary?](#how-do-i-uninstall-the-binary)
  - [What are the rules for defining or interpolating keys?](#what-are-the-rules-for-defining-or-interpolating-keys)
  - [What operating systems are supported?](#what-operating-systems-are-supported)
  - [What are the nvi configuration file specs?](#what-are-the-nvi-configuration-file-specs)
  - [Can I manually assign parsed ENVs to a process?](#can-i-manually-assign-parsed-envs-to-a-process)
  - [How do I read the debug details?](#how-do-i-read-the-debug-details)

[Troubleshooting](#troubleshooting)

[Contributing](CONTRIBUTING.md)

## Requirements

The following requirements should be present in order to build from source:
- g++ v11.3.0+
- clang or clang++ v14.0.x
- cmake v3.26.3+
- make v3.8.x+
- libcurl v8.3.x+
- clangd v14.0.x (optional for linting)
- clang-format v14.0.x (optional for formatting)

You can determine if you're using the correct versions by:
- Running `which <requirement>` that should output a binary path, if not, then it's not installed and will require platform specific installations.
- Running `<requirement> --version` that should output a binary version equal to or above the required version.

## Quick Installation

Run the following command to clone the source code (if you plan on running the unit tests, add the "--recursive" flag to download test dependencies):
```bash
git clone git@github.com:mattcarlotta/nvi.git nvi
```

Change directory:
```bash
cd nvi
```

Run the install script to build and install the binary using the default cmake parameters:
```bash
./install.sh
```

OR you can manually build and install the binary (for more cmake options, check out the [Custom CMake Compile Flags](#custom-cmake-compile-flags)):
```bash
cmake .
sudo make install
```
⚠️ Be careful when running the install script or `sudo make install` if you've also compiled the `tests` using the custom cmake `-DCOMPILE_TESTS` flag. As noted below, the flag is **OFF** by default. If the flag is active, it will install gtest dependencies to your `/usr` directory.

### Custom CMake Compile Flags

The following custom compile flags can be set for `cmake`:
- `-DCOMPILE_SRC=ON|OFF` this compiles the source files within `src` to a `nvi` binary (default: ON)
- `-DCOMPILE_TESTS=ON|OFF` this compiles the source files within `tests` to a `tests` binary (default: OFF)
- `-DINSTALL_BIN_DIR=/custom/directory/path` this will override the binary installation directory when running `sudo make install` (default: /usr/local/bin)
- `-DINSTALL_MAN_DIR=/path/to/man/man1` this will automatically install the nvi man [documentation](docs#README) to the specified directory when running `sudo make install` (default: OFF)
- `-DUSE_LOCALHOST_API=ON|OFF` this specifies to use a localhost nvi API when retrieving remote ENVs (default: OFF)

The following represents the default `cmake` settings:
```bash
cmake -DCOMPILE_SRC=ON -DCOMPILE_TESTS=OFF -DINSTALL_BIN_DIR=/usr/local/bin -DINSTALL_MAN_DIR=OFF .
```

## Custom Installations

If you would like to compile with a specific compiler, then the following instructions will allow you to build with `clang`, `clang++` or `g++` (provided they're already installed on the system):

After cloning the source code, enter the `src` directory:
```bash
cd src
```

Then, pick one of the following compiler commands:
```bash
clang -x c++ -lstdc++ -std=c++17 -lcurl *.cpp -o nvi
```
or
```bash
clang++ -std=c++17 -lcurl *.cpp -o nvi
```
or
```bash
g++ -std=c++17 -lcurl *.cpp -o nvi
```

To install the compiled binary as a system binary, you'll need to select a path recognized by your shell's `PATH` variable:
```bash
echo $PATH
```

You should see an output of paths separated by colons (`:`), for example:
```bash
/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin
```

Typically, you'll want to install the binary in `/usr/local/bin`; however, if you don't have that path on your system, see the [Troubleshooting](#troubleshooting) steps for more information.

Then, you'll either want to the move or copy the binary to a system binary path (must still be within the `src` directory):
```bash
sudo mv nvi /usr/local/bin
```
or
```bash
sudo cp nvi /usr/local/bin
```

To ensure the binary is installed, type:
```bash
which nvi
# /usr/local/bin/nvi
```

## Usage

Navigate to a project that contains one or many `.env` files, then type:
```bash
nvi <flag> <arg>
```

## Flags

All flags below are optional. Short form (`-`) and long form (`--`) flags are supported and can be mixed if desired.

If no flags are assigned, then an `.env` (that is named ".env") located at the root directory will be parsed.

- `-c` | `--config`: Specifies which environment config to load from the .nvi file. (ex: --config dev)‡
- `-de` | `--debug`: Specifies whether or not to log debug details. (ex: --debug)
- `-d` | `--dir`: Specifies which directory the .env files are located within. (ex: --dir path/to/envs)
- `-e` | `--env`: Specifies which environment config to use within a remote project. (ex: --env dev)‡‡
- `--`: Specifies which system command to run in a child process with parsed ENVs. (ex: -- cargo run)‡‡‡
- `-f` | `--files`: Specifies which .env files to parse separated by a space. (ex: --files test.env test2.env)
- `-p` | `--project`: Specifies which remote project to select from the nvi API. (ex: --project my_project)‡‡
- `-r` | `--required`: Specifies which ENV keys are required separated by a space. (ex: --required KEY1 KEY2)
- `-h` | `--help`: Displays this help information.
- `-v` | `--version`: Displays binary information.

‡ When a "-c" or "--config" flag is present, then all the other flags are ignored as they should be defined within a [configuration file](#configuration-file).

‡‡ When pulling remote ENVs from the nvi API, the "project" and "environment" flags must both be defined. 

‡‡‡ The "--" (execute) flag should be the last defined flag. Any flags after it will be consumed as part of the system command. 


## Configuration File

Instead of manually typing out flags and arguments in the CLI, there is support for placing them in a `.nvi` configuration file.

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

[remote_dev]
project = "my_project"
env = "development"
exec = "bin dev"
required = [ "TEST1", "TEST2", "TEST3" ]
```

To target a configuration within the .nvi config file, simply use the `-c` or `--config` flag followed by the config name:
```bash
nvi -c dev
# or
nvi --config staging
```

Please read [this](#what-are-the-nvi-configuration-file-specs) for config file specs.

### [Examples](/examples)

Click on the link above for language specific examples, otherwise, here are some basic use cases:

Parsing an `example.env` file from a custom directory with debug logging:
```bash
nvi --files example.env --dir dist/client --debug
```

Parsing one or many `.env` files from a [.nvi](envs/.nvi#L5-L10) configuration file typically located at a project's root directory:
```bash
nvi --config standard
```

Parsing an `.env` file, checking the parsed ENVs for required keys, and then, if good, applying those ENVs to a spawned "npm" child process:
```bash
nvi --files .env --required KEY1 KEY2 -- npm run dev
```

## Remote ENVs

To retrieve remote ENVs from the nvi API, you must first register and verify your email using the [front-facing application](https://github.com/mattcarlotta/nvi-app). 
- Once registered and verified, create a project, an environment and at least 1 ENV secret within the environment
- Navigate to your account settings page, locate your unique API key and click the copy to clipboard button
- Using the nvi CLI tool, input the following flags:
    - "-p" | "--project" followed by a space and then the name of the project you've created
    - "-e" | "--env" followed by a space and then the name of the environment you've created
    - "--" followed by a space and then a system command to run 
- Press the "Enter" key and nvi will prompt you for your unique API key
- Input the API key and nvi will attempt to retrieve and assign remote ENVs from the selected project and environment to the command (if no command is provided then nvi will just print the parsed and interpolated envs to standard out)

Retrieving remote ENVs:
```bash
nvi -p my_project -e development -- cargo run
```

Then, you'll be asked for your API key:
```bash
$ [nvi] Please enter your unique API key: 
```

Input your API key and press the "Enter" key:
```bash
$ [nvi] Please enter your unique API key: abcdefhijkhijklo0123456789
```

If no error is displayed in the terminal, then a child process should be spawned with the command OR ENVs will be printed to standard out as stringified JSON.

## FAQs

### How do I uninstall the binary?

If you'd like to remove (uninstall) the binary, simply type:
```bash
sudo rm $(which nvi)
```
If you've installed the nvi [man documentation](docs/README.md) using the custom cmake `-DINSTALL_MAN_DIR` flag, then you'll need to remove it as well:
```bash
sudo rm /path/to/man/man1/nvi.1
```

### What are the rules for defining or interpolating keys?

There are really only 3 simple rules that must be followed:
- To interpolate a key's value into another key, use `${NAME_OF_KEY}`, where `NAME_OF_KEY` represents the name of an ENV:
```bash
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

### What operating systems are supported?

Currently, GNU/Linux and Mac OS (v13+ although older versions that support C++17 may work as well). For Windows support, please visit this [documentation](https://youtu.be/BwSts2s4ba4).

### What are the nvi configuration file specs?

This `.nvi` configuration file attempts to give you the flexibility and ease-of-use of defining flags and arguments under an environment configuration. 

Therefore, while the config file parser is flexible, it is **NOT** a [TOML](https://toml.io/en/)-complaint parser and has some rules.

The configuration file must:
- be named `.nvi`
- not contain spaces within the `[environment]`'s name
- not contain spaces within a `files` .env name nor within the `required` keys; instead, files/keys should use underscores: `example_1`
- not contain comments after a configuration `key = value # comment` option
- not contain empty lines between a configuration `key = value` option; empty lines after the last config option are okay
- not contain multi-line arguments

Click [here](envs/.nvi) to view valid and invalid formatting configurations.

### Can I manually assign parsed ENVs to a process?

Yes! If you don't use a `--` or an `exec` command in a configuration file, then nvi will print out a stringified JSON result of ENVs to [stdout](https://www.computerhope.com/jargon/s/stdout.htm). 

Unfortunately, this means you'll have to manually pipe and parse stringified JSON from `stdin`, and then assign them to the process for whatever language or framework that you're using. As such, this feature is available to you, but there are expected drawbacks:
- requires language or framework specific code (what this project aims to mitigate)
- reading from `stdin` may not be possible
- reading from `stdin` may be asynchronous and there's no guarantee that when a program/process runs that the ENVs will be defined before they are used
- parsing stringified JSON will more than likely require a 3rd party package

As such, while this feature is available, it's not recommended nor going to be supported by this project. Nevertheless, here's an example of how to pipe ENVs into a [node process](/examples/node) using [stdin](/examples/node/stdin.mjs).

### How do I read the debug details?

To read the debug details, let's examine the following debug message:
```bash
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

The solution to the above is to either ensure the ENV key exists within the shell environment before running the binary or only reference keys in the .env file after they've been parsed (.env files are parsed top-down, therefore keys can only reference other keys above itself, and .env files are parsed left to right, therefore keys can only reference other keys in files that have been parsed before itself).

Not all debug logs will have all the details above, but they will generally follow this pattern.


## Troubleshooting

### The binary directory doesn't exist

Some operating systems (like Mac OS) may not have a `/usr/local/bin` directory nor use it as a search `PATH` for binaries.

To fix this, create the directory (if you're wary of touching `/usr`, then you may want to use `/opt` or `/opt/bin` instead):
```bash
sudo mkdir -p /usr/local/bin
```

Then, add this directory path to the shell's `PATH` (swap in whichever profile your shell uses, eg. `.bash_profile`, `.bashrc` or `.zsh`):
```bash
echo 'export PATH="$PATH:/usr/local/bin"' >> ~/.bash_profile
```

Then, source the change for your shell profile:
```bash
source ~/.bash_profile
```

To ensure the binary is found, type the command below and you should see the nvi binary path: 
```bash
which nvi
# /usr/local/bin/nvi
```

### Can't run any command line tools in a terminal

On older versions of Mac OS or after an OS update, you may come across this error:
```bash
xcrun: error: invalid active developer path (/Library/Developer/CommandLineTools), missing xcrun at: /Library/Developer/CommandLineTools/usr/bin/xcrun
```
To solve this error, you'll need to reinstall Command Line Tools for Xcode:
```bash
xcode-select --install
```
or visit this [link](https://developer.apple.com/download/all/?q=xcode%20command%20line%20tools) to download the lastest (preferably non-beta) Command Line Tools for Xcode.

For more debugging information, take a look at this [post](https://stackoverflow.com/questions/52522565/git-is-not-working-after-macos-update-xcrun-error-invalid-active-developer-p). 
