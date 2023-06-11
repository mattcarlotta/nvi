# Env Metrics

## Commands

Run individual tests by running the following commands:

| `yarn <command>` | Description                                                                                     |
| ---------------- | ----------------------------------------------------------------------------------------------- |
| `dotenv`         | Runs tests for `dotenv` + `dotenv-expand` (outputs results to `results.json` file as `dotenv`). |
| `noshot`         | Runs tests for `noshot` (outputs results to `results.json` file as `@noshot/env`).                |
| `nvi`            | Runs tests for `nvi` (outputs results to `results.json` file as `nvi`).                       |
| `generate:readme`| Generates a `README.md` from the `results.json` file.                                           |

⚠️ Warning: The tests can take a quite a long time to complete! Adjust the iterations or runs if needed.


## Metrics

**System Specs**:

- CPU: AMD Ryzen 9 5950x (stock)
- MOBO: Asus x570 ROG Crosshair Dark Hero
- RAM: G.Skill Trident Z Neo 32 GB (4 x 8 GB) running @ 2100Mhz
- Storage: Sabrent 1TB Rocket 4 Plus NVMe 4.0 Gen4
- OS: Linuxmint 21.1 vera
- Kernel: Linux 5.15.0-73-generic x86_64
- Node: v18.16.0

**Compiled Timestamp**: Sunday, June 11, 2023 at 3:41:37 PM

Loading and interpolating a single [small env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Sunday, June 11, 2023 at 3:15:39 PM | 500000 | 8.685s, 8.773s, 8.792s | 8.75s | 100.00% |
| @noshot/env | Sunday, June 11, 2023 at 3:25:15 PM | 500000 | 12.614s, 12.72s, 12.748s | 12.694s | 68.85% |
| dotenv | Sunday, June 11, 2023 at 3:40:57 PM | 500000 | 14.54s, 14.585s, 14.638s | 14.588s | 59.73% |

Loading and interpolating a single [large env file](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.interp):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Sunday, June 11, 2023 at 3:15:39 PM | 5000 | 16.312s, 16.595s, 16.601s | 16.503s | 100.00% |
| @noshot/env | Sunday, June 11, 2023 at 3:25:15 PM | 5000 | 54.015s, 54.841s, 56.466s | 55.107s | 30.20% |
| dotenv | Sunday, June 11, 2023 at 3:40:57 PM | 5000 | 74.317s, 74.373s, 74.414s | 74.368s | 21.95% |

Loading and interpolating multiple small env files ([1](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env), [2](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development), [3](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.local), [4](https://github.com/mattcarlotta/nvi/blob/main/benchmarks/.env.development.local)):
| package | run timestamp | iterations | duration (3 fastest runs out of 6) | avg | fastest |
| --- | --- | --- | --- | --- | --- |
| nvi | Sunday, June 11, 2023 at 3:15:39 PM | 500000 | 16.91s, 17.156s, 17.184s | 17.083s | 100.00% |
| @noshot/env | Sunday, June 11, 2023 at 3:25:15 PM | 500000 | 24.469s, 24.558s, 24.788s | 24.605s | 69.11% |
| dotenv | Sunday, June 11, 2023 at 3:40:57 PM | 500000 | 26.254s, 26.281s, 26.284s | 26.273s | 64.41% |
