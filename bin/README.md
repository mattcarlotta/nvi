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

## Build Source and Install Binary
```DOSINI
git clone git@github.com:mattcarlotta/nvi.git

cd nvi/bin

git submodule update --init --recursive

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

‡ When a "-c" or "--config" flag is present, then "debug", "dir", "exec", "files", and "required" flags are ignored as they should be defined within the "env.config.json" file.

### Examples

Example of parsing an `example.env` file from a custom directory with debug logging:
```DOSINI
nvi --files example.env --dir dist/client --debug
```

Example parsing one or many `.env` files from a [env.config.json](https://github.com/mattcarlotta/nvi/blob/main/env.config.json#L6-L11) configuration file located at a project's root directory:
```DOSINI
nvi --config bin_test_only
```

Example parsing an `.env` file, checking the parsed ENVs for required keys, and then, if good, applying those ENVs to a spawned "npm" child process:
```DOSINI
nvi --files .env --exec npm run dev --required KEY1 KEY2
```


## FAQS

### How do I uninstall the binary?
If you'd like to remove (uninstall) the binary, simply type:
```DOSINI
sudo rm $(which nvi)
```

### How do I read the debug details?
To read the debug details, let's examine the following debug message:
```DOSINI
[nvi] (parser::INTERPOLATION_WARNING::.env:21:25) The key "INTERP_ENV_FROM_PROCESS" contains an invalid interpolated variable: "USER". Unable to locate a value that corresponds to this key.
```
- Which part (file) of the binary is being executed: `parser`
- What type of log is being output: `INTERPOLATION_WARNING`
- Which file is being processed: `.env`
- Which line within the file is being processed: `21`
- Which byte within the current line is being processed: `25`
- Lastly, the debug message: `The key "..." contains an invalid interpolated variable...etc`

In layman's terms, this debug message is stating that a key's value contains an interpolated key `"${KEY}"` that doesn't match any parsed keys.

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
