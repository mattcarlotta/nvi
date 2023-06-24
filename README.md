# nvi
A custom-built .env file parser!

**Note**: This is a work in progress and will likely change over time. As such, this is **NOT** recommended for production environments yet.

## Why nvi?
✔️ Doesn't use any regular expressions

✔️ Typescript source with included type declarations

✔️ Logs warnings with `[nvi] (file_name:line:byte)` if an interpolation fails

✔️ Zero dependencies and is up to [75% faster at parsing](benchmarks/README.md#metrics) than the industry standard

✔️ Contains CJS and ESM compiled sources

✔️ Contains a stand-alone [executable](bin/README.md) to parse `.env` files to stringified JSON

✔️ Unopinionated about `.env` naming

✔️ Supports loading multiple `.env` files at once

✔️ Supports manually importing `.env` files

✔️ Supports overriding Envs in [`process.env`](https://nodejs.org/docs/latest/api/process.html#process_process_env)

✔️ Supports Env interpolation

✔️ Supports muli-line values

✔️ Supports Env preloading

✔️ Supports marking Envs as required

✔️ Supports loading Envs via an `env.config.json` file


## Contributing Guide

See [CONTRIBUTING.md](CONTRIBUTING.md)

