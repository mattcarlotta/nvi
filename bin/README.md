# nvi bin
A custom-built executable .env file parser!

**Note**: This is a work in progress and will likely change over time. As such, this is **NOT** recommended for production environments yet.

## Installation

The following requirements must be present in order to build from source:
- gcc v11.3.0+
- clang v14.0.x
- cmake v3.26.3+
- make v4.3+
- clangd v14.0.x (optional for formatting/linting)

### Build from source
```DOSINI
git clone git@github.com:mattcarlotta/nvi.git

git submodule update --init --recursive

cd nvi/bin

cmake -DCOMPILE_SRC=ON -DCOMPILE_TESTS=OFF .

make
```

### Install binary

**Option 1** - Symlink the built binary to a system binary directory (RECOMMENDED):
```DOSINI
cd src

sudo ln -s $PWD/nvi /usr/local/bin/
```

If you'd like to remove the symlink, simply type:
```DOSINI
sudo unlink /usr/local/bin/nvi
```
or
```DOSINI
sudo rm /usr/local/bin/nvi
```

**Option 2** - Copy the built binary to a system binary directory:
```DOSINI
cd src

sudo cp $PWD/nvi /usr/local/bin/
```

If you'd like to remove the binary, simply type:
```DOSINI
sudo rm /usr/local/bin/nvi
```

<details>
<summary>
⚠️ Please note that operating systems (like Mac OS) may not have a "/usr/local/bin" directory nor use it as a search PATH for binaries. Click here for a fix.
</summary>

To fix this, create the directory (you may want to use `/opt/bin` instead):
```DOSINI
sudo mkdir -p /usr/local/bin
```
Then, edit your `.bash_profile` (or edit the `.bashrc`) or edit your `.zshrc`: 
```DOSINI
vi ~/.bash_profile
```
and add the directory to the PATH variable and save:
```DOSINI
export PATH=$PATH:/usr/local/bin
```

Then, source the change:
```DOSINI
source ~/.bash_profile
```
or
```DOSINI
source ~/.bashrc
```
or
```DOSINI
source ~/.zshrc
```
</details>

To ensure the binary is found, type:
```DOSINI
which nvi
```

and you should see the nvi binary path: 
```DOSININ
/usr/local/bin/nvi
```

## Usage

Navigate to a project that contains `.env` files, then type:
```DOSINI
nvi <flag> <arg>
```

Example of parsing an `example.env` file from a custom directory with debug logging:
```DOSINI
nvi --files example.env --dir dist/client --debug
```

Example parsing one or many `.env` files from a [env.config.json](https://github.com/mattcarlotta/nvi/blob/main/env.config.json#L6-L11) configuration file located at a project's root directory:
```DOSINI
nvi -c bin_test_only
```

## Flags
All flags below are optional. 

If no flags are passed, then if present, an `.env` (that is named ".env") located at the root directory will be parsed.

| flag            | description                                                                                           |
| --------------- | ----------------------------------------------------------------------------------------------------- |
| -c, --config    | Specifies which environment configuration to load from the env.config.json file. (ex: --config dev)‡  |
| -de, --debug    | Specifies whether or not to log file parsing details. (ex: --debug)                                   |
| -d, --dir       | Specifies which directory the env file is located within. (ex: --dir path/to/env)                     |
| -f, --files     | Specifies which .env files to parse separated by a space. (ex: --files test.env test2.env)            |
| -r, --required  | Specifies which ENV keys are required separated by a space. (ex: --required KEY1 KEY2)                |
| -h, --help      | Displays this help information.                                                                       |

‡ When a '-c' or '--config' flag is present, then 'debug', 'dir', 'files', and 'required' flags are ignored as they should be defined within the "env.config.json" file.
