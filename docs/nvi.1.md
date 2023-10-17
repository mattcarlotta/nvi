% nvi(1) CLI Documentation v0.1.0
% Matt Carlotta
% 10-17-2023

# NAME

**nvi** — a stand-alone .env file parser for interpolating and assigning multiple .env files into a process.

# SYNOPSIS

| nvi \[**options**]

# DESCRIPTION

nvi is a zero-dependency, stand-alone binary that parses and interpolates .env files.
It will either print ENVs as stringified JSON to standard output or it will
assign them to a forked child process. 

All options below are optional. Only long form (\--) flags are supported.

# OPTIONS

\--api

:   Specifies whether or not to retrieve ENVs from the remote API.

\--config [environment]{.underline}

:  Specifies which environment config to load from the .nvi file. 

    When this flag is present, then the options below are ignored as they should be defined within the .nvi configuration file.

\--debug

:   Specifies whether or not to log debug details.

\--directory [path]{.underline}

:   Specifies which directory path the .env files are located within. The [path]{.underline} will be relative to the current directory.

\--environment [environment]{.underline}

:   Specifies which environment config to use within a remote project. Remote environments are created via the front-facing web application.

\--files [file1.env]{.underline} [file2.env]{.underline} [file3.env]{.underline} ...etc

:   A list of .env files to parse. Each specified [file]{.underline} needs to be separated by a space.

\--help

:   Prints a table CLI flag usage information.

\--project [project]{.underline}

:   Specifies which remote project to select from the nvi API. Remote projects are created via the front-facing web application.

\--print

:   Specifies whether or not to print ENVs to standard out. This flag will be ignored if a system command is defined.

\--required [KEY_1]{.underline} [KEY_2]{.underline} [KEY_3]{.underline} ...etc

:   A list of ENV keys that are required to exist after parsing. Each specified [KEY]{.underline} needs to be separated by a space.

\--save

:   Specifies whether or not to save remote ENVs to disk with the selected environment name.

\--version

:   Prints the current version number.

\-- [command]{.underline}

:   A system command to run in a forked child process that has been assigned with the parsed ENVs. This should be last flag defined, therefore anything placed after it will be consumed as part of the system command.

# EXIT STATUS

nvi exits 0 on success, and 1 if an error occurs.

# FILES

*\*/.nvi*

:   Global project environment configuration file.

*\*/\*.env.\**

:   One or many files containing ENVs to be parsed and interpolated.

# EXAMPLES

Parsing a local `example.env` file from a custom directory with debug logging:
```bash
$ nvi --files example.env --dir dist/client --debug
```

Parsing one or many local `.env` files from a .nvi configuration file typically located at a project's root directory:
```bash
$ nvi --config standard
```

Parsing a local `.env` file, checking the parsed ENVs for required keys, and then, if good, applying those ENVs to a spawned "npm" child process:
```bash
$ nvi --files .env --required KEY1 KEY2 -- npm run dev
```

To retrieve remote ENVs from the nvi API, you must first register and verify your email using the [front-facing application](https://github.com/mattcarlotta/nvi-app). 

1. Once registered and verified, create a project, an environment and at least 1 ENV secret within the environment
2. Navigate to your account settings page, locate your unique API key and click the copy to clipboard button
3. Using the nvi CLI tool, input the following:
    a. "\--api" flag followed by a space 
    b. optional "\--project" flag followed by a space and then the name of the project you've created
    c. optional "\--environment" flag followed by a space and then the name of the environment you've created
    d. a "\--print" flag or a \--" flag followed by a space and then a system command to run 
4. Press the "Enter" key and nvi will prompt you for your unique API key‡
5. Input the API key and nvi will attempt to retrieve and assign remote ENVs from the selected project and environment to the command (if no command is provided then nvi will just print the parsed and interpolated envs to standard out)

‡ You can bypass this step by creating a `.nvi-key` file at the project root directory that includes your unique API key. Be mindful, that this file **MUST** be added to your `.gitignore`!

Retrieving remote ENVs:
```bash
$ nvi --api --project my_project --environment development -- cargo run
```

Then, you'll be asked for your API key. Input your API key and press the "Enter" key:
```bash
$ [nvi] Please enter your unique API key: 
```

If no error is displayed in the terminal, then a child process should be spawned with the command OR ENVs will be printed to standard out as stringified JSON.

The following represents an example `.nvi` configuration:
```toml
[dev]
debug = true
directory = "path/to/custom/dir"
files = [ ".env", "base.env", "reference.env" ]
execute = "bin dev"
required = [ "TEST1", "TEST2", "TEST3" ]

[staging]
files = [ ".env" ]
required = [ "TEST1" ]

[remote_dev]
api = true
debug = true
environment = "development"
execute = "bin dev"
project = "my_project"
required = [ "TEST1", "TEST2", "TEST3" ]
save = true
```

To target a configuration within the .nvi config file, simply use the `--config` flag followed by the config name:
```bash
$ nvi --config dev
```

Please read [this](https://github.com/mattcarlotta/nvi#what-are-the-nvi-configuration-file-specs) for config file specs.

# SEE ALSO
[Source](https://github.com/mattcarlotta/nvi)

[Issues](https://github.com/mattcarlotta/nvi/issues)

[Documentation](https://github.com/mattcarlotta/nvi#README)

# LICENSE

Copyright 2023 (C) Matt Carlotta. GPL-3.0 licensed.
