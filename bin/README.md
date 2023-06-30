# nvi bin
A custom-built executable .env file parser!

**Note**: This is a work in progress and will likely change over time. As such, this is **NOT** recommended for production environments yet.

## Basic Usage:

```DOSINI
./nvi <flag> <arg>
```

Example parsing a single `.env` file from a custom directory with debug logging:
```DOSINI
./nvi --files .env --dir dist/client --debug
```

Example parsing one or many `.env` files from a `env.config.json` configuration file:
```DOSINI
./nvi --config development
```

All flags below are optional. If no flags are passed, then a single `.env` file located at the root directory will be parsed (if present).

| flag            | description                                                                                           |
| --------------- | ----------------------------------------------------------------------------------------------------- |
| -c, --config    | Specifies which environment configuration to load from the env.config.json file. (ex: --config dev)   |
| -dg, --debug    | Specifies whether or not to log file parsing details. (ex: --debug)                                   |
| -d, --dir       | Specifies which directory the env file is located within. (ex: --dir path/to/env)                     |
| -f, --files     | Specifies which .env file to load. (ex: --files test.env test2.env)                                   |
| -r, --required  | Specifies which ENV keys are required. (ex: --required KEY1 KEY2)                                     |
| -h, --help      | Displays this help information.                                                                       |

