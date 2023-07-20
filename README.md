# nvi bin
A custom-built executable .env file parser!

**Note**: This is a work in progress and will likely change over time. As such, this is **NOT** recommended for production environments yet.

## Requirements

The following requirements must be present in order to build from source:
- gcc v11.3.0+
- clang v14.0.x
- cmake v3.26.3+
- make v3.8.x+
- clangd v14.0.x (optional for formatting/linting)

You can determine if you're using the correct versions by:
- Running `which <requirement>` that should output a binary path, if not, then it's not installed and will require platform specific installations
- Running `<requirement> --version` that should output a binary version equal to or above the required version

## Copy Source, Build Source and Install Binary
```DOSINI
git clone git@github.com:mattcarlotta/nvi.git --recursive nvi

cd nvi

# if you want to use custom compile flags see below
cmake .

sudo make install
```

### Custom CMake Compile Flags

The following custom compile flags can be set for `cmake`:
- `-DCOMPILE_SRC=ON|OFF` this compiles the source files within `bin/src` to a `nvi` binary (default: ON)
- `-DCOMPILE_TESTS=ON|OFF` this compiles the source files within `bin/tests` to a `tests` binary (default: OFF)
- `-DINSTALL_BIN_DIR=/custom/directory/path` this will override the binary installation directory when running `sudo make install` (default: /usr/local/bin)

The following represents the default `cmake` settings:
```DOSINI
cmake -DCOMPILE_SRC=ON -DCOMPILE_TESTS=OFF -DINSTALL_BIN_DIR=/usr/local/bin .
```

## Usage

Navigate to a project that contains one or many `.env` files, then type:
```DOSINI
nvi <flag> <arg>
```

## Binary Flags
All flags below are optional. Short form (`-`) and long form (`--`) flags are supported and can be mixed if desired.

If no flags are assigned, then an `.env` (that is named ".env") located at the root directory will be parsed.

| flag            | description                                                                                           |
| --------------- | ----------------------------------------------------------------------------------------------------- |
| -c, --config    | Specifies which environment configuration to load from the env.config.json file. (ex: --config dev)‡  |
| -de, --debug    | Specifies whether or not to log debug details. (ex: --debug)                                          |
| -d, --dir       | Specifies which directory the env file is located within. (ex: --dir path/to/env)                     |
| -e, --exec      | Specifies which command to run in a separate process with parsed ENVS. (ex: --exec node index.js)     |
| -f, --files     | Specifies which .env files to parse separated by a space. (ex: --files test.env test2.env)            |
| -r, --required  | Specifies which ENV keys are required separated by a space. (ex: --required KEY1 KEY2)                |
| -h, --help      | Displays this help information.                                                                       |

‡ When a "-c" or "--config" flag is present, then "debug", "dir", "exec", "files", and "required" flags are ignored as they should be defined within a [configuration file](#configuration-file).


### Configuration File
Instead of manually typing out flags and arguments in the CLI, there is support for placing most of them in an `env.config.json` configuration file.

The configuration file is a [JSON](https://developer.mozilla.org/en-US/docs/Learn/JavaScript/Objects/JSON) file that contains...
- An `"environment"` name that encapsulates the following properties: 
  - debug: `true` or `false` (default: `false`) **OPTIONAL**
  - dir: `"custom/path/to/envs"` (default: `""`) **OPTIONAL**
  - execute: `"binary command"` (default: `""`) **OPTIONAL**
  - files: `["1.env", "2.env", "3.env"]` (default `[]`) **REQUIRED**
  - required: `["KEY_1", "KEY_2", "KEY_3"]` (default `[]`) **OPTIONAL**

The following represents an example `env.config.json` configuration:
```json
{
    "dev": {
        "debug": true,
        "dir": "custom/directory/to/envs",
        "execute": "bin test",
        "files": ["1.env", "2.env", "3.env"],
        "required": ["KEY_1", "KEY_2", "KEY_3"]
    },
    "test": {
        "files": ["test.env"],
        "required": ["TEST_KEY_1", "TEST_KEY_2", "TEST_KEY_3"]
    }

}
```

To target an environment within the configuration file, simply use the `-c` or `--config` flag followed by the environment name:
```DOSINI
nvi -c dev
# or
nvi --config test
```

### [Examples](/examples)

Click on the link above for language specific examples, otherwise, here are some basic use cases:

Parsing an `example.env` file from a custom directory with debug logging:
```DOSINI
nvi --files example.env --dir dist/client --debug
```

Parsing one or many `.env` files from a [env.config.json](https://github.com/mattcarlotta/nvi/blob/main/env.config.json#L6-L12) configuration file located at a project's root directory:
```DOSINI
nvi --config bin_test_only
```

Parsing an `.env` file, checking the parsed ENVs for required keys, and then, if good, applying those ENVs to a spawned "npm" child process:
```DOSINI
nvi --files .env --exec npm run dev --required KEY1 KEY2
```

## FAQS

### How do I uninstall the binary?
If you'd like to remove (uninstall) the binary, simply type:
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

### Can I manually assign parsed ENVs to a process?
Yes! If you don't use an `-e` or `--exec` or `execute` command, then nvi will print out a stringified JSON result of ENVS to [stdout](https://www.computerhope.com/jargon/s/stdout.htm). 

Unfortunately, this means you'll have to manually pipe, parse stringified JSON from `stdin`, and assign them to the process for whatever language or framework that you're using. As such, this feature is available to you, but there are expected drawbacks:
- requires language or framework specific code (what this project aims to mitigate)
- reading from `stdin` may not be possible
- reading from `stdin` may be asynchronous and there's no guarantee that when a program/process runs that the ENVs will be defined before they are used
- parsing stringified JSON will more than likely require a 3rd party package

As such, while this feature is available, it's not recommended nor going to be supported by this project. Nevertheless, here's an example of how to pipe ENVs into a [node process](/examples/node) using [stdin](/examples/node/stdin.mjs).

### How do I read the debug details?
To read the debug details, let's examine the following debug message:
```DOSINI
[nvi] (parser::INTERPOLATION_WARNING::.env:21:25) The key "INTERP_ENV_FROM_PROCESS" contains an invalid interpolated variable: "TEST". Unable to locate a value that corresponds to this key.
```
- Which part (file) of the binary is being executed: `parser`
- What type of log is being output: `INTERPOLATION_WARNING`
- Which file is being processed: `.env`
- Which line within the file is being processed: `21`
- Which byte within the current line is being processed: `25`
- Lastly, the debug message: `The key "..." contains an invalid interpolated variable...etc`

In layman's terms, this debug message is stating that a key's value contains an interpolated key `${KEY}` (eg. TEST) that doesn't match any ENV keys in the process nor any previously parsed ENV keys.

The solution to the above is to either ensure the ENV key exists within the process before running the binary or only reference keys in the .env file after they've been parsed (.env files are parsed top-down, therefore keys can only reference other keys above itself).

Not all debug logs will have all the details above, but will generally follow the same pattern.


## Troubleshooting

⚠️ Please note that some operating systems (like Mac OS) may not have a "/usr/local/bin" directory nor use it as a search `PATH` for binaries.

To fix this, create the directory (if you're wary of touching `/usr/`, then you may want to use `/opt` or `/opt/bin` instead):
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

To ensure the binary is found, type the command below and you should see the nvi binary path: 
```DOSINI
which nvi
# /usr/local/bin/nvi
```
