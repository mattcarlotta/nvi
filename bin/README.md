# nvi bin
A custom-built executable .env file reader!

**Note**: This is a work in progress and will likely change over time. As such, this is **NOT** recommended for production environments yet.

## Basic Usage:

```DOSINI
./nvi <flag> <arg>
```

Example parsing a single `.env` file from a custom directory with debug logging:
```DOSINI
./nvi -file .env -dir dist/client -debug on
```

Example parsing one or many `.env` files from a `env.config.json` configuration file:
```DOSINI
./nvi -config development
```

All flags below are optional. If no flags are passed, then a single `.env` file located at the root directory will be parsed (if present).

| flag    | description                                                                                                   |
| ------- | ------------------------------------------------------------------------------------------------------------- |
| -config | Specifies which environment configuration you'd like to load from the env.config.json file. (ex: -config dev) |
| -debug  | Specifies whether or not to log file parsing details. (ex: -debug on/off)                                     |
| -dir    | Specifies which directory the env file is located within. (ex: -dir path/to/env)                              |
| -file   | Specifies which env file you'd like to load. (ex: -file test.env)                                             |
| -help   | Displays this help information                                                                                |

