% nvi(1) CLI Documentation v0.0.1 
% Matt Carlotta
% 07-31-2023

# NAME

**nvi** — a stand-alone .env file parser for interpolating and assigning multiple .env files into a process.

# SYNOPSIS

| nvi \[**options**]

# DESCRIPTION

nvi is a zero-dependency, stand-alone binary that parses and interpolates .env files.
It will either print ENVs as stringified JSON to standard output or it will
assign them to a forked child process. 

All options below are optional. Short form (-) and long form (\--) flags are supported 
and can be mixed if desired.

If no flags are assigned, then an .env (that is named ".env") located at the current 
directory will be parsed and printed to standard out.

# OPTIONS

-c, \--config [environment]{.underline}

:  Specifies which environment config to load from the .nvi file. 

    When this flag is present, then the options below are ignored as they should be defined within the .nvi configuration file.

-de, \--debug

:   Specifies whether or not to log debug details.

-d, \--dir [path]{.underline}

:   Specifies which directory path the .env files are located within. The [path]{.underline} will be relative to the current directory.

-e, \--exec [command]{.underline}

:   A command to run in a forked child process that has been assigned with the parsed ENVs.

-f, \--files [file]{.underline}

:   A list of .env files to parse. Each specified [file]{.underline} needs to be separated by a space.

-h, \--help

:   Prints brief CLI flag usage information.

-r, \--required [KEY]{.underline}

:   A list of ENV keys are that required to exist after parsing. Each specified [KEY]{.underline} needs to be separated by a space.

-v, \--version

:   Prints the current version number.

# EXIT STATUS

nvi exits 0 on success, and 1 if an error occurs.

# FILES

*\*/.nvi*

:   Global project environment configuration file.

*\*/\*.env.\**

:   One or many files containing ENVs to be parsed and interpolated.

# EXAMPLES

Parsing an `example.env` file from a custom directory with debug logging:
```bash
$ nvi --files example.env --dir dist/client --debug
```

Parsing one or many `.env` files from a .nvi configuration file typically located at a project's root directory:
```bash
$ nvi --config standard
```

Parsing an `.env` file, checking the parsed ENVs for required keys, and then, if good, applying those ENVs to a spawned "npm" child process:
```bash
$ nvi --files .env --exec npm run dev --required KEY1 KEY2
```

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
```bash
$ nvi -c dev
```
or
```bash
$ nvi --config staging
```

Please read [this](https://github.com/mattcarlotta/nvi#what-are-the-nvi-configuration-file-specs) for config file specs.

# SEE ALSO
[Source](https://github.com/mattcarlotta/nvi)

[Issues](https://github.com/mattcarlotta/nvi/issues)

[Documentation](https://github.com/mattcarlotta/nvi#README)

# LICENSE

Copyright 2023 (C) Matt Carlotta. GPL-3.0 licensed.
